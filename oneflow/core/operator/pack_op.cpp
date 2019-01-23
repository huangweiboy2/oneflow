#include "oneflow/core/operator/pack_op.h"

namespace oneflow {

void PackOp::InitFromOpConf() {
  CHECK(op_conf().has_pack_conf());

  EnrollInputBn("in", false);
  EnrollOutputBn("out", false);
}

void PackOp::InferOutputBlobTimeShape(
    std::function<const Shape*(const std::string&)> GetTimeShape4BnInOp,
    const ParallelContext* parallel_ctx, Shape* time_shape) const {
  std::vector<int64_t> dim_vec(GetTimeShape4BnInOp("in")->dim_vec());
  int32_t pack_num = GetPackNum();
  CHECK_EQ(pack_num, dim_vec.back());
  dim_vec.pop_back();
  *time_shape = Shape(dim_vec);
}

int32_t PackOp::GetPackNum() const {
  CHECK(op_conf().has_pack_conf());
  const PackOpConf& conf = op_conf().pack_conf();
  return conf.pack_num();
}

REGISTER_OP(OperatorConf::kPackConf, PackOp);

}  // namespace oneflow
