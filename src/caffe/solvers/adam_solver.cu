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
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>("m",
                    KERNEL_ARG_GLOBAL_MEM));
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>("v",
                    KERNEL_ARG_GLOBAL_MEM));
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "beta1", KERNEL_ARG_CONST));
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "beta2", KERNEL_ARG_CONST));
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                    "eps_hat",  KERNEL_ARG_CONST));
  args.push_back(this->device_program_->template create_kernel_arg<Dtype>(
                 "corrected_local_rate", KERNEL_ARG_CONST));
  ss << this->device_program_->function("AdamUpdate", args);
  ss << this->device_program_->kernel_loop("uint_tp", "i", "n");
  ss << "Dtype gi = g[i];" << std::endl;
  ss << "Dtype mi = m[i] = m[i] * beta1 + gi * (1 - beta1);" << std::endl;
  ss << "Dtype vi = v[i] = v[i] * beta2 + gi * gi * (1 - beta2);" << std::endl;
  ss << "g[i] = corrected_local_rate * mi / (sqrt(vi) + eps_hat);" << std::endl;
  ss << "}" << std::endl;
  ss << "}" << std::endl;

  this->device_program_->set_source(ss.str());
  this->device_program_->Compile(true, true);
}

template<typename Dtype>
void adam_update_gpu(Device* dev, DeviceProgram* dev_prog, uint_tp n,
                     vptr<Dtype> g, vptr<Dtype> m, vptr<Dtype> v,
                     Dtype beta1, Dtype beta2, Dtype eps_hat,
                     Dtype corrected_local_rate) {
  shared_ptr<DeviceKernel> kernel = dev_prog->GetKernel("AdamUpdate");
  kernel->add_arg(&n);
  kernel->add_arg(&g);
  kernel->add_arg(&m);
  kernel->add_arg(&v);
  kernel->add_arg(&beta1);
  kernel->add_arg(&beta2);
  kernel->add_arg(&eps_hat);
  kernel->add_arg(&corrected_local_rate);

  vector<size_t> work_size(1, n);
  vector<size_t> group;
  vector<size_t> local;
  dev->get_threads(&work_size, &group, &local, kernel.get(), true);
  kernel->Execute(group, local);
}

#ifdef USE_GPU_HALF
template void adam_update_gpu<half_float::half>(Device* dev,
              DeviceProgram* dev_prog, uint_tp n, vptr<half_float::half> g,
              vptr<half_float::half> m, vptr<half_float::half> v,
              half_float::half beta1, half_float::half beta2,
              half_float::half eps_hat, half_float::half corrected_local_rate);
#endif  // USE_GPU_HALF
#ifdef USE_GPU_SINGLE
template void adam_update_gpu<float>(Device*,
              DeviceProgram* dev_prog, uint_tp n, vptr<float> g,
              vptr<float> m, vptr<float> v,
              float beta1, float beta2,
              float eps_hat, float corrected_local_rate);
#endif  // USE_GPU_SINGLE
#ifdef USE_GPU_DOUBLE
template void adam_update_gpu<double>(Device* dev,
              DeviceProgram* dev_prog, uint_tp n, vptr<double> g,
              vptr<double> m, vptr<double> v,
              double beta1, double beta2,
              double eps_hat, double corrected_local_rate);
#endif  // USE_GPU_DOUBLE

}  // namespace caffe
