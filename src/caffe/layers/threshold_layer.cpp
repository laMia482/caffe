#include <vector>

#include "caffe/layers/threshold_layer.hpp"

namespace caffe {

template<typename Dtype, typename MItype, typename MOtype>
void ThresholdLayer<Dtype, MItype, MOtype>::LayerSetUp(
                                          const vector<Blob<MItype>*>& bottom,
                                          const vector<Blob<MOtype>*>& top) {
  NeuronLayer<Dtype, MItype, MOtype>::LayerSetUp(bottom, top);
  threshold_ = this->layer_param_.threshold_param().threshold();
}

template<typename Dtype, typename MItype, typename MOtype>
void ThresholdLayer<Dtype, MItype, MOtype>::Reshape(
      const vector<Blob<MItype>*>& bottom,
      const vector<Blob<MOtype>*>& top) {
  NeuronLayer<Dtype, MItype, MOtype>::Reshape(bottom, top);

  if (Caffe::mode() == Caffe::GPU && this->device_program_.get() == nullptr) {
    this->GenerateProgram<Dtype, MItype, MOtype>();
  }
}

template<typename Dtype, typename MItype, typename MOtype>
void ThresholdLayer<Dtype, MItype, MOtype>::Forward_cpu(
                                          const vector<Blob<MItype>*>& bottom,
                                          const vector<Blob<MOtype>*>& top) {
  const Dtype* bottom_data = bottom[0]->cpu_data();
  Dtype* top_data = top[0]->mutable_cpu_data();
  const int_tp count = bottom[0]->count();
  for (int_tp i = 0; i < count; ++i) {
    top_data[i] = (bottom_data[i] > threshold_) ? Dtype(1) : Dtype(0);
  }
}

#ifdef CPU_ONLY
STUB_GPU_FORWARD(ThresholdLayer, Forward);
#endif

INSTANTIATE_CLASS_3T(ThresholdLayer);
REGISTER_LAYER_CLASS(Threshold);

}  // namespace caffe
