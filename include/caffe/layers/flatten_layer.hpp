#ifndef CAFFE_FLATTEN_LAYER_HPP_
#define CAFFE_FLATTEN_LAYER_HPP_

#include <vector>

#include "caffe/blob.hpp"
#include "caffe/layer.hpp"
#include "caffe/proto/caffe.pb.h"

namespace caffe {

/**
 * @brief Reshapes the input Blob int_tpo flat vectors.
 *
 * Note: because this layer does not change the input values -- merely the
 * dimensions -- it can simply copy the input. The copy happens "virtually"
 * (thus taking effectively 0 real time) by setting, in Forward, the data
 * point_tper of the top Blob to that of the bottom Blob (see Blob::ShareData),
 * and in Backward, the diff point_tper of the bottom Blob to that of the top
 * Blob (see Blob::ShareDiff).
 */
template<typename Dtype, typename MItype, typename MOtype>
class FlattenLayer : public Layer<Dtype, MItype, MOtype> {
 public:
  explicit FlattenLayer(const LayerParameter& param)
      : Layer<Dtype, MItype, MOtype>(param) {}
  virtual void Reshape(const vector<Blob<MItype>*>& bottom,
      const vector<Blob<MOtype>*>& top);

  virtual inline const char* type() const { return "Flatten"; }
  virtual inline int_tp ExactNumBottomBlobs() const { return 1; }
  virtual inline int_tp ExactNumTopBlobs() const { return 1; }

 protected:
  /**
   * @param bottom input Blob vector (length 2+)
   *   -# @f$ (n \times c \times H \times W) @f$
   *      the inputs
   * @param top output Blob vector (length 1)
   *   -# @f$ (n \times CHW \times 1 \times 1) @f$
   *      the outputs -- i.e., the (virtually) copied, flattened inputs
   */
  virtual void Forward_cpu(const vector<Blob<MItype>*>& bottom,
      const vector<Blob<MOtype>*>& top);

  /**
   * @brief Computes the error gradient w.r.t. the concatenate inputs.
   *
   * @param top output Blob vector (length 1), providing the error gradient with
   *        respect to the outputs
   * @param propagate_down see Layer::Backward.
   * @param bottom input Blob vector (length k), int_tpo which the top error
   *        gradient is (virtually) copied
   */
  virtual void Backward_cpu(const vector<Blob<MOtype>*>& top,
      const vector<bool>& propagate_down,
      const vector<Blob<MItype>*>& bottom);
};

}  // namespace caffe

#endif  // CAFFE_FLATTEN_LAYER_HPP_
