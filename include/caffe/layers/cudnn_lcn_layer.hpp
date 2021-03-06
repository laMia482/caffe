#ifndef CAFFE_CUDNN_LCN_LAYER_HPP_
#define CAFFE_CUDNN_LCN_LAYER_HPP_

#include <vector>

#include "caffe/blob.hpp"
#include "caffe/layer.hpp"
#include "caffe/proto/caffe.pb.h"

#include "caffe/layers/lrn_layer.hpp"
#include "caffe/layers/power_layer.hpp"

#ifdef USE_CUDNN  // cuDNN acceleration library.
#include "caffe/util/cudnn.hpp"
#endif

namespace caffe {

#ifdef USE_CUDNN
template<typename Dtype, typename MItype, typename MOtype>
class CuDNNLCNLayer : public LRNLayer<Dtype, MItype, MOtype> {
 public:
  explicit CuDNNLCNLayer(const LayerParameter& param)
      : LRNLayer<Dtype, MItype, MOtype>(param), handles_setup_(false),
        tempDataSize(0), tempData1(NULL), tempData2(NULL) {}
  virtual void LayerSetUp(const vector<Blob<MItype>*>& bottom,
      const vector<Blob<MOtype>*>& top);
  virtual void Reshape(const vector<Blob<MItype>*>& bottom,
      const vector<Blob<MOtype>*>& top);
  virtual ~CuDNNLCNLayer();

 protected:
  virtual void Forward_gpu(const vector<Blob<MItype>*>& bottom,
      const vector<Blob<MOtype>*>& top);
  virtual void Backward_gpu(const vector<Blob<MOtype>*>& top,
      const vector<bool>& propagate_down, const vector<Blob<MItype>*>& bottom);

  bool handles_setup_;
  cudnnHandle_t handle_;
  cudnnLRNDescriptor_t norm_desc_;
  cudnnTensorDescriptor_t bottom_desc_, top_desc_;

  int_tp size_, pre_pad_;
  Dtype alpha_, beta_, k_;

  size_t tempDataSize;
  void *tempData1, *tempData2;
};
#endif

}  // namespace caffe

#endif  // CAFFE_CUDNN_LCN_LAYER_HPP_
