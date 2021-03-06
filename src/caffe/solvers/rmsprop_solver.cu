#include "caffe/backend/device.hpp"
#include "caffe/util/math_functions.hpp"
#include "caffe/sgd_solvers.hpp"


namespace caffe {

template<typename Dtype>
void AdamSolver<Dtype>::GenerateProgram() {
  this->device_program_ = this->device_->CreateProgram();
  stringstream ss;

  ss << this->device_program_->setup();
  ss << this->device_program_->template define_type<Dtype>("Dtype");

  KernelArgs args;
  args.push_back(this->device_program_->template create_kernel_arg<uint_tp>("n",
                    KERNEL_ARG_CONST));
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>("g",
                    KERNEL_ARG_GLOBAL_MEM));
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>("h",
                    KERNEL_ARG_GLOBAL_MEM));
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "rms_decay", KERNEL_ARG_CONST));
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "delta", KERNEL_ARG_CONST));
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "local_rate",  KERNEL_ARG_CONST));
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                 "corrected_local_rate", KERNEL_ARG_CONST));
  ss << this->device_program_->function("RMSPropUpdate", args);
  ss << this->device_program_->kernel_loop("uint_tp", "i", "n");
  ss << "Dtype gi = g[i];" << std::endl;
  ss << "Dtype hi = h[i] = rms_decay * h[i] + (1 - rms_decay) * gi * gi;"
     << std::endl;
  ss << "g[i] = local_rate * g[i] / (sqrt(hi) + delta);" << std::endl;
  ss << "}" << std::endl;
  ss << "}" << std::endl;

  this->device_program_->set_source(ss.str());
  this->device_program_->Compile(true, true);
}

template <typename Dtype>
void rmsprop_update_gpu(Device* dev, DeviceProgram* dev_prog, uint_tp n,
                        vptr<Dtype> g, vptr<Dtype> h, Dtype rms_decay,
                        Dtype delta, Dtype local_rate) {
  shared_ptr<DeviceKernel> kernel = dev_prog->GetKernel("RMSPropUpdate");
  kernel->add_arg(&n);
  kernel->add_arg(&g);
  kernel->add_arg(&h);
  kernel->add_arg(&rms_decay);
  kernel->add_arg(&delta);
  kernel->add_arg(&local_rate);

  vector<size_t> work_size(1, n);
  vector<size_t> group;
  vector<size_t> local;
  dev->get_threads(&work_size, &group, &local, kernel.get(), true);
  kernel->Execute(group, local);
}

#ifdef USE_GPU_HALF
template void rmsprop_update_gpu<half_float::half>(Device* dev,
              DeviceProgram* dev_prog, uint_tp n, vptr<half_float::half> g,
              vptr<half_float::half> h, half_float::half rms_decay,
              half_float::half delta, half_float::half local_rate);
#endif  // USE_GPU_HALF
#ifdef USE_GPU_SINGLE
template void rmsprop_update_gpu<float>(Device* dev,
              DeviceProgram* dev_prog, uint_tp n, vptr<float> g,
              vptr<float> h, float rms_decay, float delta,
              float local_rate);
#endif  // USE_GPU_HALF
#ifdef USE_GPU_DOUBLE
template void rmsprop_update_gpu<double>(Device* dev,
              DeviceProgram* dev_prog, uint_tp n, vptr<double> g,
              vptr<double> h, double rms_decay, double delta,
              double local_rate);
#endif  // USE_GPU_DOUBLE

}  // namespace caffe
