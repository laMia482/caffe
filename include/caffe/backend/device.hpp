#ifndef CAFFE_BACKEND_DEVICE_HPP_
#define CAFFE_BACKEND_DEVICE_HPP_

#ifdef CMAKE_BUILD
#include "caffe_config.h"
#endif

#include <string>
#include <vector>
#include "caffe/blob.hpp"
#include "caffe/backend/backend.hpp"
#include "caffe/backend/device_program.hpp"
#include "caffe/backend/device_kernel.hpp"
#include "caffe/backend/vptr.hpp"

namespace caffe {

class Device {
 public:
  explicit Device();

  Backend backend() const;
  uint_tp id() const;
  uint_tp list_id() const;

  template<typename Dtype>
  shared_ptr<Blob<Dtype> > Buffer(uint_tp id);

  uint_tp memory_usage();
  uint_tp peak_memory_usage();
  void increase_memory_usage(uint_tp bytes);
  void decrease_memory_usage(uint_tp bytes);
  void reset_peak_memory_usage();

  virtual void Init();
  virtual bool CheckCapability(string cap);
  virtual bool CheckVendor(string vendor);
  virtual bool CheckType(string type);
  virtual void SwitchQueue(uint_tp id);
  virtual uint_tp current_queue_id();
  size_t workgroup_size(uint_tp id);
  virtual void get_threads(const vector<size_t>* work_size,
                           vector<size_t>* local,
                           vector<size_t>* group,
                           DeviceKernel* kernel,
                           bool auto_select);
  virtual void FinishQueues();
  virtual uint_tp num_queues();
  virtual bool is_host_unified();
  bool is_fast_unsafe_math() const;
  virtual string name();
  virtual shared_ptr<DeviceProgram> CreateProgram();

  virtual void MallocMemHost(void** ptr, uint_tp size);
  virtual void FreeMemHost(void* ptr);
  virtual vptr<void> MallocMemDevice(uint_tp size, void** ptr,
                                     bool zero_copy);
  virtual void FreeMemDevice(vptr<void> ptr);
  virtual bool CheckZeroCopy(vptr<const void> gpu_ptr, void* cpu_ptr,
                             uint_tp size);

  template<typename Dtype>
  void im2col(vptr<const Dtype> data_im, const int_tp channels,
              const int_tp height, const int_tp width, const int_tp kernel_h,
              const int_tp kernel_w, const int_tp pad_h, const int_tp pad_w,
              const int_tp stride_h, const int_tp stride_w,
              const int_tp dilation_h, const int_tp dilation_w,
              vptr<Dtype> data_col);

  template<typename Dtype>
  void col2im(vptr<const Dtype> data_col, const int_tp channels,
              const int_tp height, const int_tp width, const int_tp kernel_h,
              const int_tp kernel_w, const int_tp pad_h, const int_tp pad_w,
              const int_tp stride_h, const int_tp stride_w,
              const int_tp dilation_h, const int_tp dilation_w,
              vptr<Dtype> data_im);

  template<typename Dtype>
  void im2col_nd(vptr<const Dtype> data_im, const int_tp num_spatial_axes,
                 const int_tp num_kernels, vptr<const int_tp> im_shape,
                 vptr<const int_tp> col_shape, vptr<const int_tp> kernel_shape,
                 vptr<const int_tp> pad, vptr<const int_tp> stride,
                 vptr<const int_tp> dilation, vptr<Dtype> data_col);

  template<typename Dtype>
  void col2im_nd(vptr<const Dtype> data_col, const int_tp num_spatial_axes,
                 const int_tp im_size, vptr<const int_tp> im_shape,
                 vptr<const int_tp> col_shape, vptr<const int_tp> kernel_shape,
                 vptr<const int_tp> pad, vptr<const int_tp> stride,
                 vptr<const int_tp> dilation, vptr<Dtype> data_im);


  template<typename Dtype>
  void copy(const uint_tp n, vptr<const Dtype> x, vptr<Dtype> y);

  template<typename Dtype>
  void copy(const uint_tp n, const Dtype* x, vptr<Dtype> y);

  template<typename Dtype>
  void copy(const uint_tp n, vptr<const Dtype> x, Dtype* y);

  template<typename Dtype>
  void gemm(const CBLAS_TRANSPOSE trans_a, const CBLAS_TRANSPOSE trans_b,
                      const uint_tp m, const uint_tp n, const uint_tp k,
                      const Dtype alpha, vptr<const Dtype> a,
                      vptr<const Dtype> b,
                      const Dtype beta, vptr<Dtype> c);

  template<typename Dtype>
  void gemv(const CBLAS_TRANSPOSE trans_a, const uint_tp m,
                      const uint_tp n, const Dtype alpha,
                      vptr<const Dtype> a,
                      vptr<const Dtype> x, const Dtype beta,
                      vptr<Dtype> y);

  template<typename Dtype>
  void axpy(const uint_tp n, const Dtype alpha, vptr<const Dtype> x,
            vptr<Dtype> y);

  template<typename Dtype>
  void axpby(const uint_tp n, const Dtype alpha, vptr<const Dtype> x,
             const Dtype beta, vptr<Dtype> y);

  virtual void memcpy(const uint_tp n, vptr<const void> x, vptr<void> y);
  virtual void memcpy(const uint_tp n, const void* x, vptr<void> y);
  virtual void memcpy(const uint_tp n, vptr<const void> x, void* y);

  template<typename Dtype>
  void set(const uint_tp n, const Dtype alpha, vptr<Dtype> x);

  virtual void memset(const uint_tp n, const char alpha, vptr<char> x);

  template<typename Dtype>
  void add_scalar(const uint_tp n, const Dtype alpha, vptr<Dtype> x);

  template<typename Dtype>
  void scal(const uint_tp n, const Dtype alpha, vptr<Dtype> x);

  template<typename Dtype>
  void add(const uint_tp n, vptr<const Dtype> a, vptr<const Dtype> b,
           vptr<Dtype> y);

  template<typename Dtype>
  void sub(const uint_tp n, vptr<const Dtype> a, vptr<const Dtype> b,
           vptr<Dtype> y);

  template<typename Dtype>
  void mul(const uint_tp n, vptr<const Dtype> a, vptr<const Dtype> b,
           vptr<Dtype> y);

  template<typename Dtype>
  void div(const uint_tp n, vptr<const Dtype> a, vptr<const Dtype> b,
           vptr<Dtype> y);

  template<typename Dtype>
  void abs(const uint_tp n, vptr<const Dtype> a, vptr<Dtype> y);

  template<typename Dtype>
  void exp(const uint_tp n, vptr<const Dtype> a, vptr<Dtype> y);

  template<typename Dtype>
  void log(const uint_tp n, vptr<const Dtype> a, vptr<Dtype> y);

  template<typename Dtype>
  void powx(const uint_tp n, vptr<const Dtype> a, const Dtype b, vptr<Dtype> y);

  template <typename Dtype>
  void sqrt(const uint_tp n, vptr<const Dtype> a, vptr<Dtype> y);

  // rng_uniform with two arguments generates integers in the range
  // [0, UINT_MAX].
  virtual void rng_uniform(const uint_tp n, vptr<uint32_t> r);  // NOLINT
  virtual void rng_uniform(const uint_tp n, vptr<uint64_t> r);  // NOLINT

  // rng_uniform with four arguments generates floats in the range
  // (a, b] (strictly greater than a, less than or equal to b)
  template<typename Dtype>
  void rng_uniform(const uint_tp n, const Dtype a, const Dtype b,
                   vptr<Dtype> r);

  template<typename Dtype>
  void rng_gaussian(const uint_tp n, const Dtype mu, const Dtype sigma,
                    vptr<Dtype> r);

  template<typename Dtype>
  void rng_bernoulli(const uint_tp n, const Dtype p, vptr<int> r);

  template<typename Dtype>
  void rng_bernoulli(const uint_tp n, const Dtype p, vptr<unsigned int> r);

  template<typename Dtype>
  void dot(const uint_tp n, vptr<const Dtype> x, vptr<const Dtype> y,
           Dtype* out);

  template<typename Dtype>
  void asum(const uint_tp n, vptr<const Dtype> x, Dtype* y);

  template<typename Dtype>
  void sign(const uint_tp n, vptr<const Dtype> x, vptr<Dtype> y);

  template<typename Dtype>
  void sgnbit(const uint_tp n, vptr<const Dtype> x, vptr<Dtype> y);

  template<typename Dtype>
  void scale(const uint_tp n, const Dtype alpha, vptr<const Dtype> x,
             vptr<Dtype> y);

 protected:

  void CreateMathProgram();
  void CreateIm2ColProgram();

  virtual void gemm_half
                (const CBLAS_TRANSPOSE trans_a, const CBLAS_TRANSPOSE trans_b,
                 const uint_tp m, const uint_tp n, const uint_tp k,
                 const half_float::half alpha, vptr<const half_float::half> a,
                 vptr<const half_float::half> b,
                 const half_float::half beta,
                 vptr<half_float::half> c);

  virtual void gemm_float
                (const CBLAS_TRANSPOSE trans_a, const CBLAS_TRANSPOSE trans_b,
                 const uint_tp m, const uint_tp n, const uint_tp k,
                 const float alpha, vptr<const float> a,
                 vptr<const float> b,
                 const float beta, vptr<float> c);

  virtual void gemm_double
                (const CBLAS_TRANSPOSE trans_a, const CBLAS_TRANSPOSE trans_b,
                 const uint_tp m, const uint_tp n, const uint_tp k,
                 const double alpha, vptr<const double> a,
                 vptr<const double> b,
                 const double beta, vptr<double> c);

  virtual void gemv_half
                (const CBLAS_TRANSPOSE trans_a, const uint_tp m,
                 const uint_tp n, const half_float::half alpha,
                 vptr<const half_float::half> a,
                 vptr<const half_float::half> x, const half_float::half beta,
                 vptr<half_float::half> y);

  virtual void gemv_float
                (const CBLAS_TRANSPOSE trans_a, const uint_tp m,
                 const uint_tp n, const float alpha,
                 vptr<const float> a,
                 vptr<const float> x, const float beta,
                 vptr<float> y);

  virtual void gemv_double
                (const CBLAS_TRANSPOSE trans_a, const uint_tp m,
                 const uint_tp n, const double alpha,
                 vptr<const double> a,
                 vptr<const double> x, const double beta,
                 vptr<double> y);

  virtual void axpy_half(const uint_tp n,
                         const half_float::half alpha,
                         vptr<const half_float::half> x,
                         vptr<half_float::half> y);

  virtual void axpy_float(const uint_tp n, const float alpha,
                          vptr<const float> x, vptr<float> y);

  virtual void axpy_double(const uint_tp n, const double alpha,
                          vptr<const double> x, vptr<double> y);

  virtual void axpby_half(const uint_tp n, const half_float::half alpha,
                     vptr<const half_float::half> x,
                     const half_float::half beta, vptr<half_float::half> y);

  virtual void axpby_float(const uint_tp n, const float alpha,
                   vptr<const float> x, const float beta, vptr<float> y);

  virtual void axpby_double(const uint_tp n, const double alpha,
                   vptr<const double> x, const double beta, vptr<double> y);

  virtual void rng_uniform_half(const uint_tp n, const half_float::half a,
                        const half_float::half b, vptr<half_float::half> r);

  virtual void rng_uniform_float(const uint_tp n, const float a, const float b,
                                 vptr<float> r);

  virtual void rng_uniform_double(const uint_tp n, const double a,
                                  const double b, vptr<double> r);

  virtual void rng_gaussian_half(const uint_tp n, const half_float::half mu,
                    const half_float::half sigma, vptr<half_float::half> r);

  virtual void rng_gaussian_float(const uint_tp n, const float mu,
                                  const float sigma, vptr<float> r);

  virtual void rng_gaussian_double(const uint_tp n, const double mu,
                                   const double sigma, vptr<double> r);

  virtual void rng_bernoulli_half(const uint_tp n, const half_float::half p,
                                  vptr<int> r);

  virtual void rng_bernoulli_float(const uint_tp n, const float p,
                                   vptr<int> r);

  virtual void rng_bernoulli_double(const uint_tp n, const double p,
                                    vptr<int> r);

  virtual void rng_bernoulli_half(const uint_tp n, const half_float::half p,
                                  vptr<unsigned int> r);

  virtual void rng_bernoulli_float(const uint_tp n, const float p,
                                   vptr<unsigned int> r);

  virtual void rng_bernoulli_double(const uint_tp n, const double p,
                                    vptr<unsigned int> r);

  virtual void dot_half(const uint_tp n, vptr<const half_float::half> x,
                     vptr<const half_float::half> y, half_float::half *out);

  virtual void dot_float(const uint_tp n, vptr<const float> x,
                         vptr<const float> y, float *out);

  virtual void dot_double(const uint_tp n, vptr<const double> x,
                          vptr<const double> y, double *out);

  virtual void asum_half(const uint_tp n, vptr<const half_float::half> x,
                         half_float::half* y);

  virtual void asum_float(const uint_tp n, vptr<const float> x, float* y);

  virtual void asum_double(const uint_tp n, vptr<const double> x,
                           double* y);

  virtual void scal_half(const uint_tp n, const half_float::half alpha,
                         vptr<half_float::half> x);

  virtual void scal_float(const uint_tp n, const float alpha,
                         vptr<float> x);

  virtual void scal_double(const uint_tp n, const double alpha,
                         vptr<double> x);

  virtual void scale_half(const uint_tp n, const half_float::half alpha,
                          vptr<const half_float::half> x,
                          vptr<half_float::half> y);

  virtual void scale_float(const uint_tp n, const float alpha,
                           vptr<const float> x, vptr<float> y);

  virtual void scale_double(const uint_tp n, const double alpha,
                            vptr<const double> x, vptr<double> y);

  int current_queue_id_;
  size_t max_local_size_;
  vector<size_t> max_local_sizes_;
  vector<size_t> max_group_sizes_;
  int id_;
  int list_id_;
  Backend backend_;
  uint_tp memory_usage_;
  uint_tp peak_memory_usage_;
  vector<shared_ptr<Blob<half_float::half> > > buff_h_;
  vector<shared_ptr<Blob<float> > > buff_f_;
  vector<shared_ptr<Blob<double> > > buff_d_;
  bool host_unified_;
  bool fast_unsafe_math_;
  string name_;
  shared_ptr<DeviceProgram> math_program_;
  shared_ptr<DeviceProgram> im2col_program_;
};

}  // namespace caffe

#endif  // CAFFE_BACKEND_DEVICE_HPP_
