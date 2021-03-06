#include <climits>
#include <vector>

#include "caffe/blob.hpp"

#include "caffe/backend/device.hpp"
#include "caffe/common.hpp"
#include "caffe/syncedmem.hpp"
#include "caffe/util/math_functions.hpp"

namespace caffe {

template<typename Dtype>
bool Blob<Dtype>::Reshape(const uint_tp num, const uint_tp channels,
                          const uint_tp height, const uint_tp width) {
  vector<uint_tp> shape(4);
  shape[0] = num;
  shape[1] = channels;
  shape[2] = height;
  shape[3] = width;
  return Reshape(shape, shape);
}

template<typename Dtype>
bool Blob<Dtype>::Reshape(const vector<uint_tp>& shape) {
  return Reshape(shape, shape);
}

template<typename Dtype>
bool Blob<Dtype>::Reshape(const vector<uint_tp>& shape,
                                 const vector<uint_tp>& shape_stride) {
  CHECK_LE(shape.size(), kMaxBlobAxes);
  count_ = 1;
  shape_.resize(shape.size());
  if (!shape_data_ || shape_data_->size() < shape.size() * sizeof(uint_tp)) {
    shape_data_.reset(
        new SyncedMemory(shape.size() * sizeof(uint_tp), device_));
  }
  uint_tp* shape_data = static_cast<uint_tp*>(shape_data_->mutable_cpu_data());
  for (uint_tp i = 0; i < shape.size(); ++i) {
    CHECK_GE(shape[i], 0);
    if (count_ != 0) {
#ifdef USE_INDEX_64
      CHECK_LE(shape[i], LONG_MAX / count_) << "blob size exceeds INT_MAX";
#else
      CHECK_LE(shape[i], INT_MAX / count_) << "blob size exceeds INT_MAX";
#endif  // USE_INDEX_64
    }
    count_ *= shape[i];
    shape_[i] = shape[i];
    shape_data[i] = shape[i];
  }
  if (count_ > capacity_) {
    capacity_ = count_;
    data_.reset(new SyncedMemory(capacity_ * sizeof(Dtype), device_));
    diff_.reset(new SyncedMemory(capacity_ * sizeof(Dtype), device_));
    return true;
  }
  return false;
}

template<typename Dtype>
bool Blob<Dtype>::Reshape(const BlobShape& shape,
                                 const BlobShape& shape_stride) {
  CHECK_LE(shape.dim_size(), kMaxBlobAxes);
  vector<uint_tp> shape_vec(shape.dim_size());
  vector<uint_tp> shape_stride_vec(shape.dim_size());
  for (uint_tp i = 0; i < shape.dim_size(); ++i) {
    shape_vec[i] = shape.dim(i);
    shape_stride_vec[i] = shape_stride.dim(i);
  }
  return Reshape(shape_vec, shape_stride_vec);
}

template<typename Dtype>
bool Blob<Dtype>::Reshape(const BlobShape& shape) {
  CHECK_LE(shape.dim_size(), kMaxBlobAxes);
  vector<uint_tp> shape_vec(shape.dim_size());
  for (uint_tp i = 0; i < shape.dim_size(); ++i) {
    shape_vec[i] = shape.dim(i);
  }
  return Reshape(shape_vec, shape_vec);
}

template<typename Dtype>
bool Blob<Dtype>::ReshapeLike(const Blob<Dtype>& other) {
  return Reshape(other.shape());
}

template<typename Dtype>
Blob<Dtype>::Blob(const uint_tp num, const uint_tp channels,
                  const uint_tp height, const uint_tp width,
                  Device *device_context)
    // capacity_ must be initialized before calling Reshape
    : capacity_(0), device_(device_context) {
  Reshape(num, channels, height, width);
}

template<typename Dtype>
Blob<Dtype>::Blob(const vector<uint_tp>& shape, Device *device_context)
    // capacity_ must be initialized before calling Reshape
    : capacity_(0), device_(device_context) {
  Reshape(shape);
}

template <typename Dtype>
vptr<const uint_tp> Blob<Dtype>::gpu_shape() const {
  CHECK(shape_data_);
  return shape_data_->gpu_data();
}

template <typename Dtype>
const Dtype* Blob<Dtype>::cpu_data() const {
  CHECK(data_);
  return (const Dtype*) data_->cpu_data();
}

template<typename Dtype>
void Blob<Dtype>::set_cpu_data(Dtype* data) {
  CHECK(data);
  // Make sure CPU and GPU sizes remain equal
  size_t size = count_ * sizeof(Dtype);
  if (data_->size() != size) {
    data_.reset(new SyncedMemory(size, device_));
    diff_.reset(new SyncedMemory(size, device_));
  }
  data_->set_cpu_data(data);
}

template<typename Dtype>
vptr<const Dtype> Blob<Dtype>::gpu_data() const {
  CHECK(data_);
  return data_->gpu_data();
}

template <typename Dtype>
void Blob<Dtype>::set_gpu_data(vptr<Dtype> data) {
  // Make sure CPU and GPU sizes remain equal
  size_t size = count_ * sizeof(Dtype);
  if (data_->size() != size) {
    data_.reset(new SyncedMemory(size, device_));
    diff_.reset(new SyncedMemory(size, device_));
  }
  data_->set_gpu_data(data);
}

template <typename Dtype>
const Dtype* Blob<Dtype>::cpu_diff() const {
  CHECK(diff_);
  return (const Dtype*) diff_->cpu_data();
}

template<typename Dtype>
vptr<const Dtype> Blob<Dtype>::gpu_diff() const {
  CHECK(diff_);
  return diff_->gpu_data();
}

template<typename Dtype>
Dtype* Blob<Dtype>::mutable_cpu_data() {
  CHECK(data_);
  return static_cast<Dtype*>(data_->mutable_cpu_data());
}

template<typename Dtype>
vptr<Dtype> Blob<Dtype>::mutable_gpu_data() {
  CHECK(data_);
  return vptr<Dtype>(data_->mutable_gpu_data());
}

template<typename Dtype>
Dtype* Blob<Dtype>::mutable_cpu_diff() {
  CHECK(diff_);
  return static_cast<Dtype*>(diff_->mutable_cpu_data());
}

template<typename Dtype>
vptr<Dtype> Blob<Dtype>::mutable_gpu_diff() {
  CHECK(diff_);
  return diff_->mutable_gpu_data();
}

template<typename Dtype>
void Blob<Dtype>::ShareData(const Blob& other) {
  CHECK_EQ(count_, other.count());
  data_ = other.data();
}

template<typename Dtype>
void Blob<Dtype>::ShareDiff(const Blob& other) {
  CHECK_EQ(count_, other.count());
  diff_ = other.diff();
}

// The "update" method is used for parameter blobs in a Net, which are stored
// as Blob<float> or Blob<double> -- hence we do not define it for
// Blob<uint_tp> or Blob<uint_tp>.
template<> void Blob<uint_tp>::Update() {
  NOT_IMPLEMENTED;
}
template<> void Blob<int_tp>::Update() {
  NOT_IMPLEMENTED;
}

template<typename Dtype>
void Blob<Dtype>::Update() {
  // We will perform update based on where the data is located.
  switch (data_->head()) {
    case SyncedMemory::HEAD_AT_CPU: {
      // perform computation on CPU
      caffe_axpy<Dtype>(count_, Dtype(-1),
                        static_cast<const Dtype*>(diff_->cpu_data()),
                        static_cast<Dtype*>(data_->mutable_cpu_data()));

      break;
    }
    case SyncedMemory::HEAD_AT_GPU:
    case SyncedMemory::SYNCED: {
#ifndef CPU_ONLY
      // perform computation on GPU
      device_->axpy<Dtype>(count_, Dtype(-1), diff_->gpu_data(),
                           data_->mutable_gpu_data());
#else
      NO_GPU;
#endif
      break;
    }
    default:
      LOG(FATAL)<< "Syncedmem not initialized.";
  }
}

template<> uint_tp Blob<uint_tp>::asum_data() const {
  NOT_IMPLEMENTED;
  return 0;
}

template<> int_tp Blob<int_tp>::asum_data() const {
  NOT_IMPLEMENTED;
  return 0;
}

template<typename Dtype>
Device *Blob<Dtype>::get_device() {
  return device_;
}

template<typename Dtype>
Dtype Blob<Dtype>::asum_data() const {
  if (!data_) {
    return (Dtype)0;
  }
  switch (data_->head()) {
    case SyncedMemory::HEAD_AT_CPU:
      return caffe_cpu_asum(count_, cpu_data());
    case SyncedMemory::HEAD_AT_GPU:
    case SyncedMemory::SYNCED: {
#ifndef CPU_ONLY
      Dtype asum;
      device_->asum<Dtype>(count_, gpu_data(), &asum);
      return asum;
#else
      NO_GPU;
#endif
    }
    case SyncedMemory::UNINITIALIZED:
      return 0;
    default:
      LOG(FATAL)<< "Unknown SyncedMemory head state: " << data_->head();
  }
  return 0;
}

template<> uint_tp Blob<uint_tp>::asum_diff() const {
  NOT_IMPLEMENTED;
  return 0;
}

template<> int_tp Blob<int_tp>::asum_diff() const {
  NOT_IMPLEMENTED;
  return 0;
}

template<typename Dtype>
Dtype Blob<Dtype>::asum_diff() const {
  if (!diff_) {
    return 0;
  }
  switch (diff_->head()) {
    case SyncedMemory::HEAD_AT_CPU:
      return caffe_cpu_asum(count_, cpu_diff());
    case SyncedMemory::HEAD_AT_GPU:
    case SyncedMemory::SYNCED: {
#ifndef CPU_ONLY
      Dtype asum;
      device_->asum<Dtype>(count_, gpu_diff(), &asum);
      return asum;
#else
      NO_GPU;
#endif
    }
    case SyncedMemory::UNINITIALIZED:
      return 0;
    default:
      LOG(FATAL)<< "Unknown SyncedMemory head state: " << diff_->head();
    }
  return 0;
}

template<> uint_tp Blob<uint_tp>::sumsq_data() const {
  NOT_IMPLEMENTED;
  return 0;
}

template<> int_tp Blob<int_tp>::sumsq_data() const {
  NOT_IMPLEMENTED;
  return 0;
}

template<typename Dtype>
Dtype Blob<Dtype>::sumsq_data() const {
  Dtype sumsq;
  Dtype gpu_sumsq;
  const Dtype* data;
  vptr<const Dtype> gpu_vptr_data;
  if (!data_) {
    return 0;
  }
  switch (data_->head()) {
    case SyncedMemory::HEAD_AT_CPU: {
      data = cpu_data();
      sumsq = caffe_cpu_dot(count_, data, data);
      break;
    }
    case SyncedMemory::HEAD_AT_GPU:
    case SyncedMemory::SYNCED: {
#ifndef CPU_ONLY
      gpu_vptr_data = gpu_data();
      device_->dot<Dtype>(count_, gpu_vptr_data, gpu_vptr_data, &gpu_sumsq);
      sumsq = gpu_sumsq;
#else
      NO_GPU;
#endif
      break;
    }
    case SyncedMemory::UNINITIALIZED:
      return 0;
    default:
      LOG(FATAL)<< "Unknown SyncedMemory head state: " << data_->head();
    }
  return sumsq;
}

template<> uint_tp Blob<uint_tp>::sumsq_diff() const {
  NOT_IMPLEMENTED;
  return 0;
}

template<> int_tp Blob<int_tp>::sumsq_diff() const {
  NOT_IMPLEMENTED;
  return 0;
}

template<typename Dtype>
Dtype Blob<Dtype>::sumsq_diff() const {
  Dtype sumsq;
  Dtype gpu_sumsq;
  const Dtype* diff;
  vptr<Dtype> gpu_diff;
  if (!diff_) {
    return 0;
  }
  switch (diff_->head()) {
    case SyncedMemory::HEAD_AT_CPU: {
      diff = cpu_diff();
      sumsq = caffe_cpu_dot(count_, diff, diff);
      break;
    }
    case SyncedMemory::HEAD_AT_GPU:
    case SyncedMemory::SYNCED: {
#ifndef CPU_ONLY
      device_->dot<Dtype>(count_, gpu_diff, gpu_diff, &gpu_sumsq);
      sumsq = gpu_sumsq;
#else
      NO_GPU;
#endif
      break;
    }
    case SyncedMemory::UNINITIALIZED:
      return 0;
    default:
      LOG(FATAL)<< "Unknown SyncedMemory head state: " << data_->head();
    }
  return sumsq;
}

template<> void Blob<uint_tp>::scale_data(uint_tp scale_factor) {
  NOT_IMPLEMENTED;
}

template<> void Blob<int_tp>::scale_data(int_tp scale_factor) {
  NOT_IMPLEMENTED;
}

template<typename Dtype>
void Blob<Dtype>::scale_data(Dtype scale_factor) {
  Dtype* data;
  vptr<Dtype> gpu_data;
  if (!data_) {
    return;
  }
  switch (data_->head()) {
    case SyncedMemory::HEAD_AT_CPU: {
      data = mutable_cpu_data();
      caffe_scal(count_, scale_factor, data);
      return;
    }
    case SyncedMemory::HEAD_AT_GPU:
    case SyncedMemory::SYNCED: {
#ifndef CPU_ONLY
      gpu_data = mutable_gpu_data();
      device_->scal<Dtype>(count_, scale_factor, gpu_data);
      return;
#else
      NO_GPU;
#endif
    }
    case SyncedMemory::UNINITIALIZED:
      return;
    default:
      LOG(FATAL)<< "Unknown SyncedMemory head state: " << data_->head();
    }
  }

template<> void Blob<uint_tp>::scale_diff(uint_tp scale_factor) {
  NOT_IMPLEMENTED;
}

template<> void Blob<int_tp>::scale_diff(int_tp scale_factor) {
  NOT_IMPLEMENTED;
}

template<typename Dtype>
void Blob<Dtype>::scale_diff(Dtype scale_factor) {
  Dtype* diff;
  vptr<Dtype> gpu_vptr_diff;
  if (!diff_) {
    return;
  }
  switch (diff_->head()) {
    case SyncedMemory::HEAD_AT_CPU: {
      diff = mutable_cpu_diff();
      caffe_scal(count_, scale_factor, diff);
      return;
    }
    case SyncedMemory::HEAD_AT_GPU:
    case SyncedMemory::SYNCED: {
#ifndef CPU_ONLY
      gpu_vptr_diff = mutable_gpu_diff();
      device_->scal<Dtype>(count_, scale_factor, gpu_vptr_diff);
      return;
#else
      NO_GPU;
#endif
    }
    case SyncedMemory::UNINITIALIZED:
      return;
    default:
      LOG(FATAL)<< "Unknown SyncedMemory head state: " << diff_->head();
    }
  }

template<typename Dtype>
bool Blob<Dtype>::ShapeEquals(const BlobProto& other) {
  if (other.has_num() || other.has_channels() || other.has_height()
      || other.has_width()) {
    // Using deprecated 4D Blob dimensions --
    // shape is (num, channels, height, width).
    // Note: we do not use the normal Blob::num(), Blob::channels(), etc.
    // methods as these index from the beginning of the blob shape, where legacy
    // parameter blobs were indexed from the end of the blob shape (e.g., bias
    // Blob shape (1 X 1 X 1 X n), IP layer weight Blob shape (1 X 1 X m X n)).
    return shape_.size() <= 4 && LegacyShape(-4) == other.num()
        && LegacyShape(-3) == other.channels()
        && LegacyShape(-2) == other.height()
        && LegacyShape(-1) == other.width();
  }
  vector<uint_tp> other_shape(other.shape().dim_size());
  for (uint_tp i = 0; i < other.shape().dim_size(); ++i) {
    other_shape[i] = other.shape().dim(i);
  }
  return shape_ == other_shape;
}

template<typename Dtype>
void Blob<Dtype>::CopyFrom(const Blob& source, bool copy_diff, bool reshape) {
  if (source.count() != count_ || source.shape() != shape_) {
    if (reshape) {
      ReshapeLike(source);
    } else {
      LOG(FATAL)<< "Trying to copy blobs of different sizes.";
    }
  }
  switch (Caffe::mode()) {
    case Caffe::GPU: {
      if (copy_diff) {
        device_->copy(count_, source.gpu_diff(),
                      vptr<Dtype>(diff_->mutable_gpu_data()));
      } else {
        device_->copy(count_, source.gpu_data(),
                      vptr<Dtype>(data_->mutable_gpu_data()));
      }
      break;
    }
    case Caffe::CPU: {
      if (copy_diff) {
        caffe_cpu_copy(count_, source.cpu_diff(),
            static_cast<Dtype*>(diff_->mutable_cpu_data()));
      } else {
        caffe_cpu_copy(count_, source.cpu_data(),
            static_cast<Dtype*>(data_->mutable_cpu_data()));
      }
      break;
    }
    default:
    LOG(FATAL)<< "Unknown caffe mode.";
  }
}

template<typename Dtype>
void Blob<Dtype>::FromProto(const BlobProto& proto, bool reshape) {
  if (reshape) {
    vector<uint_tp> shape;
    if (proto.has_num() || proto.has_channels() || proto.has_height()
        || proto.has_width()) {
      // Using deprecated 4D Blob dimensions --
      // shape is (num, channels, height, width).
      shape.resize(4);
      shape[0] = proto.num();
      shape[1] = proto.channels();
      shape[2] = proto.height();
      shape[3] = proto.width();
    } else {
      shape.resize(proto.shape().dim_size());
      for (uint_tp i = 0; i < proto.shape().dim_size(); ++i) {
        shape[i] = proto.shape().dim(i);
      }
    }
    Reshape(shape);
  } else {
    CHECK(ShapeEquals(proto)) << "shape mismatch (reshape not set)";
  }
  // copy data
  Dtype* data_vec = mutable_cpu_data();
  if (proto.double_data_size() > 0) {
    CHECK_EQ(count_, proto.double_data_size());
    for (uint_tp i = 0; i < count_; ++i) {
      data_vec[i] = proto.double_data(i);
    }
  } else {
    CHECK_EQ(count_, proto.data_size());
    for (uint_tp i = 0; i < count_; ++i) {
      data_vec[i] = proto.data(i);
    }
  }
  if (proto.double_diff_size() > 0) {
    CHECK_EQ(count_, proto.double_diff_size());
    Dtype* diff_vec = mutable_cpu_diff();
    for (uint_tp i = 0; i < count_; ++i) {
      diff_vec[i] = proto.double_diff(i);
    }
  } else if (proto.diff_size() > 0) {
    CHECK_EQ(count_, proto.diff_size());
    Dtype* diff_vec = mutable_cpu_diff();
    for (uint_tp i = 0; i < count_; ++i) {
      diff_vec[i] = proto.diff(i);
    }
  }
}

template<>
void Blob<half_float::half>::ToProto(BlobProto* proto, bool write_diff) const {
  proto->clear_shape();
  for (uint_tp i = 0; i < shape_.size(); ++i) {
    proto->mutable_shape()->add_dim(shape_[i]);
  }
  proto->clear_double_data();
  proto->clear_double_diff();
  const half_float::half* data_vec = cpu_data();
  for (uint_tp i = 0; i < count_; ++i) {
    proto->add_double_data(data_vec[i]);
  }
  if (write_diff) {
    const half_float::half* diff_vec = cpu_diff();
    for (uint_tp i = 0; i < count_; ++i) {
      proto->add_double_diff(diff_vec[i]);
    }
  }
}

template<>
void Blob<double>::ToProto(BlobProto* proto, bool write_diff) const {
  proto->clear_shape();
  for (uint_tp i = 0; i < shape_.size(); ++i) {
    proto->mutable_shape()->add_dim(shape_[i]);
  }
  proto->clear_double_data();
  proto->clear_double_diff();
  const double* data_vec = cpu_data();
  for (uint_tp i = 0; i < count_; ++i) {
    proto->add_double_data(data_vec[i]);
  }
  if (write_diff) {
    const double* diff_vec = cpu_diff();
    for (uint_tp i = 0; i < count_; ++i) {
      proto->add_double_diff(diff_vec[i]);
    }
  }
}

template<>
void Blob<float>::ToProto(BlobProto* proto, bool write_diff) const {
  proto->clear_shape();
  for (uint_tp i = 0; i < shape_.size(); ++i) {
    proto->mutable_shape()->add_dim(shape_[i]);
  }
  proto->clear_data();
  proto->clear_diff();
  const float* data_vec = cpu_data();
  for (uint_tp i = 0; i < count_; ++i) {
    proto->add_data(data_vec[i]);
  }
  if (write_diff) {
    const float* diff_vec = cpu_diff();
    for (uint_tp i = 0; i < count_; ++i) {
      proto->add_diff(diff_vec[i]);
    }
  }
}

INSTANTIATE_CLASS_1T(Blob);
template class Blob<int_tp>;
template class Blob<uint_tp>;

}  // namespace caffe

