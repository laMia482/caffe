#include <vector>

#include "caffe/layers/sigmoid_cross_entropy_loss_layer.hpp"
#include "caffe/util/math_functions.hpp"

namespace caffe {

template<typename Dtype, typename MItype, typename MOtype>
void SigmoidCrossEntropyLossLayer<Dtype, MItype, MOtype>::GenerateProgram() {
  this->device_program_ = this->device_->CreateProgram();
  stringstream ss;

  ss << this->device_program_->setup();
  ss << this->device_program_->template define_type<Dtype>("Dtype");
  ss << this->device_program_->template define_type<MItype>("MItype");
  ss << this->device_program_->template define_type<MOtype>("MOtype");

  KernelArgs fw_args;
  fw_args.push_back(this->device_program_->template create_kernel_arg<uint_tp>(
                    "count", KERNEL_ARG_CONST));
  fw_args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "input_data", KERNEL_ARG_CONST | KERNEL_ARG_GLOBAL_MEM));
  fw_args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "target", KERNEL_ARG_CONST | KERNEL_ARG_GLOBAL_MEM));
  fw_args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "loss", KERNEL_ARG_GLOBAL_MEM));
  fw_args.push_back(this->device_program_->template create_kernel_arg<bool>(
                    "has_ingore_label", KERNEL_ARG_CONST));
  fw_args.push_back(this->device_program_->template create_kernel_arg<int_tp>(
                    "ingore_label", KERNEL_ARG_CONST));
  fw_args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "counts", KERNEL_ARG_GLOBAL_MEM));
  ss << this->device_program_->function("SigmoidCrossEntropyLossForwardGPU",
                                        fw_args);
  ss << this->device_program_->kernel_loop("uint_tp", "index", "count");
  ss << "const int_tp target_value = (int_tpc)(target[i]);"
     << std::endl;
  ss << "if (has_ignore_label && target_value == ignore_label) {" << std::endl;
  ss << "loss[i] = 0;" << std::endl;
  ss << "counts[i] = 0;" << std::endl;
  ss << "} else {" << std::endl;
  ss << "loss[i] = input_data[i] * (target[i] - (input_data[i] >= 0)) -"
     << " log(1 + exp(input_data[i] - 2 * input_data[i] *"
     << " (input_data[i] >= 0)));" << std::endl;
  ss << "counts[i] = 1;" << std::endl;
  ss << "}" << std::endl;
  ss << "}" << std::endl;
  ss << "}" << std::endl;

  KernelArgs bw_args;
  bw_args.push_back(this->device_program_->template create_kernel_arg<uint_tp>(
                    "count", KERNEL_ARG_CONST));
  bw_args.push_back(this->device_program_->template create_kernel_arg<int_tp>(
                    "ignore_label", KERNEL_ARG_CONST));
  bw_args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "target", KERNEL_ARG_CONST | KERNEL_ARG_GLOBAL_MEM));
  bw_args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "diff", KERNEL_ARG_GLOBAL_MEM));
  ss << this->device_program_->function("SigmoidCrossEntropyLossIgnoreDiffGPU",
                                        bw_args);
  ss << this->device_program_->kernel_loop("uint_tp", "index", "nthreads");
  ss << "const int_tp target_value = static_cast<int_tp>(target[i]);"
     << std::endl;
  ss << "if (target_value == ignore_label) {" << std::endl;
  ss << "diff[i] = 0;" << std::endl;
  ss << "}" << std::endl;
  ss << "}" << std::endl;
  ss << "}" << std::endl;

  this->device_program_->set_source(ss.str());
  this->device_program_->Compile(true, true);
}

template<typename Dtype, typename MItype, typename MOtype>
void SigmoidCrossEntropyLossLayer<Dtype, MItype, MOtype>::Forward_gpu(
    const vector<Blob<MItype>*>& bottom,
    const vector<Blob<MOtype>*>& top) {
  // The forward pass computes the sigmoid outputs.
  sigmoid_bottom_vec_[0] = bottom[0];
  sigmoid_layer_->Forward(sigmoid_bottom_vec_, sigmoid_top_vec_);
  // Compute the loss (negative log likelihood)
  const int_tp count = bottom[0]->count();
  const int_tp num = bottom[0]->num();
  // Stable version of loss computation from input data
  vptr<const Dtype> input_data = bottom[0]->gpu_data();
  vptr<const Dtype> target = bottom[1]->gpu_data();
  // Since this memory is not used for anything until it is overwritten
  // on the backward pass, we use it here to avoid having to allocate new GPU
  // memory to accumulate intermediate results in the kernel.
  vptr<Dtype> loss_data = bottom[0]->mutable_gpu_diff();

  Dtype loss;
  vptr<Dtype> count_data = bottom[1]->mutable_gpu_diff();
  Dtype valid_count;

  shared_ptr<DeviceKernel> kernel =
          this->device_program_->GetKernel("SigmoidCrossEntropyLossForwardGPU");
  kernel->add_arg(&count);
  kernel->add_arg(&input_data);
  kernel->add_arg(&target);
  kernel->add_arg(&loss_data);
  kernel->add_arg(&has_ignore_label_);
  kernel->add_arg(&ignore_label_);
  kernel->add_arg(&count_data);

  vector<size_t> work_size(1, count);
  vector<size_t> group;
  vector<size_t> local;
  this->device_->get_threads(&work_size, &group, &local, kernel.get(), true);
  kernel->Execute(group, local);

  this->device_->template asum<Dtype>(count, loss_data, &loss);
  // Only launch another CUDA kernel if we actually need the valid count.
  if (normalization_ == LossParameter_NormalizationMode_VALID &&
      has_ignore_label_) {
    this->device_->template asum<Dtype>(count, count_data, &valid_count);
  } else {
    valid_count = count;
  }
  this->device_->template asum<Dtype>(count, loss_data, &loss);
  normalizer_ = get_normalizer(normalization_, valid_count);
  top[0]->mutable_cpu_data()[0] = loss / normalizer_;
}

template<typename Dtype, typename MItype, typename MOtype>
void SigmoidCrossEntropyLossLayer<Dtype, MItype, MOtype>::Backward_gpu(
    const vector<Blob<MOtype>*>& top, const vector<bool>& propagate_down,
    const vector<Blob<MItype>*>& bottom) {
  if (propagate_down[1]) {
    LOG(FATAL)<< this->type()
    << " Layer cannot backpropagate to label inputs.";
  }
  if (propagate_down[0]) {
    const int_tp count = bottom[0]->count();
    vptr<const Dtype> sigmoid_output_data = sigmoid_output_->gpu_data();
    vptr<const Dtype> target = bottom[1]->gpu_data();
    vptr<Dtype> bottom_diff = bottom[0]->mutable_gpu_diff();

    // First, compute the diff
    this->device_->template copy<Dtype>(count, sigmoid_output_data,
                                        bottom_diff);
    this->device_->template axpy<Dtype>(count, Dtype(-1), target, bottom_diff);
    // Zero out gradient of ignored targets.
    if (has_ignore_label_) {

      shared_ptr<DeviceKernel> kernel =
       this->device_program_->GetKernel("SigmoidCrossEntropyLossIgnoreDiffGPU");
      kernel->add_arg(&count);
      kernel->add_arg(&ignore_label_);
      kernel->add_arg(&target);
      kernel->add_arg(&bottom_diff);

      vector<size_t> work_size(1, count);
      vector<size_t> group;
      vector<size_t> local;
      this->device_->get_threads(&work_size, &group, &local, kernel.get(),
                                 true);
      kernel->Execute(group, local);
    }
    // Scale down gradient
    Dtype loss_weight = top[0]->cpu_diff()[0] / normalizer_;
    this->device_->template scal<Dtype>(count, loss_weight, bottom_diff);
  }
}

INSTANTIATE_LAYER_GPU_FUNCS(SigmoidCrossEntropyLossLayer);

}  // namespace caffe
