#ifndef ONEFLOW_CORE_KERNEL_PROPOSAL_TARGET_KERNEL_H_
#define ONEFLOW_CORE_KERNEL_PROPOSAL_TARGET_KERNEL_H_

#include "oneflow/core/kernel/kernel.h"
#include "oneflow/core/kernel/bbox_util.h"

namespace oneflow {

template<typename T>
class ProposalTargetKernel final : public KernelIf<DeviceType::kCPU> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(ProposalTargetKernel);
  ProposalTargetKernel() = default;
  ~ProposalTargetKernel() = default;

  using BBox = IndexedBBoxT<T>;
  using RoiBBox = IndexedBBoxT<const T>;
  using GtBBox = BBoxT<const T>;
  using RoiBoxIndices = BBoxIndices<IndexSequence, RoiBBox>;
  using MaxOverlapOfRoiBoxWithGt = MaxOverlapIndices<RoiBoxIndices>;
  using GtBoxIndices = BBoxIndices<IndexSequence, GtBBox>;
  using LabeledGtBox = LabelIndices<GtBoxIndices>;

 private:
  void ForwardDataContent(const KernelCtx&,
                          std::function<Blob*(const std::string&)>) const override;
  void ForwardDim0ValidNum(const KernelCtx& ctx,
                           std::function<Blob*(const std::string&)> BnInOp2Blob) const override;
  void ForwardRecordIdInDevicePiece(
      const KernelCtx& ctx, std::function<Blob*(const std::string&)> BnInOp2Blob) const override;

  void InitializeOutputBlob(DeviceCtx* ctx,
                            const std::function<Blob*(const std::string&)>& BnInOp2Blob) const;
  auto GetImageGtBoxes(const std::function<Blob*(const std::string&)>& BnInOp2Blob) const
      -> LabeledGtBox;
  auto GetImageRoiBoxes(const std::function<Blob*(const std::string&)>& BnInOp2Blob) const
      -> MaxOverlapOfRoiBoxWithGt;
  void FindNearestGtBoxForEachRoiBox(const std::function<Blob*(const std::string&)>& BnInOp2Blob,
                                     const LabeledGtBox& gt_boxes,
                                     MaxOverlapOfRoiBoxWithGt& roi_boxes) const;
  void SubsampleForegroundAndBackground(const std::function<Blob*(const std::string&)>& BnInOp2Blob,
                                        const LabeledGtBox& gt_boxes,
                                        MaxOverlapOfRoiBoxWithGt& boxes) const;
  void Output(const std::function<Blob*(const std::string&)>& BnInOp2Blob,
              const LabeledGtBox& gt_boxes, const MaxOverlapOfRoiBoxWithGt& boxes) const;
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_KERNEL_PROPOSAL_TARGET_KERNEL_H_
