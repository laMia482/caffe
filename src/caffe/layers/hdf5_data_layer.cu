#ifdef USE_HDF5
/*
 TODO:
 - only load parts of the file, in accordance with a prototxt param "max_mem"
 */

#include <stdint.h>
#include <vector>

#include "hdf5.h"
#include "hdf5_hl.h"

#include "caffe/layers/hdf5_data_layer.hpp"

namespace caffe {

template<typename Dtype, typename MItype, typename MOtype>
void HDF5DataLayer<Dtype, MItype, MOtype>::Forward_gpu(
                                         const vector<Blob<MItype>*>& bottom,
                                         const vector<Blob<MOtype>*>& top) {
  const int_tp batch_size = this->layer_param_.hdf5_data_param().batch_size();
  for (int_tp i = 0; i < batch_size; ++i) {
    while (Skip()) {
      Next();
    }
    for (int j = 0; j < this->layer_param_.top_size(); ++j) {
      int data_dim = top[j]->count() / top[j]->shape(0);
      vptr<Dtype> top_data = top[j]->mutable_gpu_data() + i * data_dim;
      this->device_->template copy<Dtype>(data_dim,
         &hdf_blobs_[j]->cpu_data()[data_permutation_[current_row_] * data_dim],
         top_data);
    }
    Next();
  }
}

INSTANTIATE_LAYER_GPU_FUNCS(HDF5DataLayer);

}  // namespace caffe
#endif  // USE_HDF5
