#include "oneflow/core/common/flat_msg_view.h"
#include "oneflow/core/vm/stream_type.h"
#include "oneflow/core/vm/instruction.msg.h"
#include "oneflow/core/vm/stream.msg.h"
#include "oneflow/core/vm/thread_ctx.msg.h"
#include "oneflow/core/vm/cuda_instruction_status_querier.h"
#include "oneflow/core/vm/cuda_stream_handle_device_context.h"
#include "oneflow/core/device/cuda_util.h"
#include "oneflow/core/common/util.h"
#include "oneflow/core/kernel/kernel_util.h"
#include "oneflow/core/job/resource.pb.h"

namespace oneflow {
namespace vm {

class CudaCopyD2HStreamType final : public StreamType {
 public:
  CudaCopyD2HStreamType() = default;
  ~CudaCopyD2HStreamType() = default;

  void InitDeviceCtx(std::unique_ptr<DeviceCtx>* device_ctx, Stream* stream) const override;

  void InitInstructionStatus(const Stream& stream,
                             InstructionStatusBuffer* status_buffer) const override;
  void DeleteInstructionStatus(const Stream& stream,
                               InstructionStatusBuffer* status_buffer) const override;
  bool QueryInstructionStatusDone(const Stream& stream,
                                  const InstructionStatusBuffer& status_buffer) const override;
  void Compute(InstrChain* instr_chain) const override;
  ObjectMsgPtr<StreamDesc> MakeRemoteStreamDesc(const Resource& resource,
                                                int64_t this_machine_id) const override;
};

namespace {

// clang-format off
FLAT_MSG_VIEW_BEGIN(CudaCopyD2HInstruction);
  FLAT_MSG_VIEW_DEFINE_PATTERN(MutableMirroredObjectOperand, dst);
  FLAT_MSG_VIEW_DEFINE_PATTERN(ConstMirroredObjectOperand, src);
  FLAT_MSG_VIEW_DEFINE_PATTERN(uint64_t, size);
FLAT_MSG_VIEW_END(CudaCopyD2HInstruction);
// clang-format on

void CudaCopyD2H(Instruction* instr) {
  void* dst = nullptr;
  const void* src = nullptr;
  size_t size = 0;
  const auto& stream = instr->mut_instr_chain()->stream();
  {
    FlatMsgView<CudaCopyD2HInstruction> view;
    CHECK(view->Match(instr->mut_instr_msg()->mut_operand()));
    size = view->size();
    auto* dst_mirrored_obj =
        instr->FindMirroredObjectByOperand(view->dst().operand(), stream.parallel_id());
    CHECK_NOTNULL(dst_mirrored_obj);
    dst = dst_mirrored_obj->mut_host_mem_buffer()->mut_data();
    auto* src_mirrored_obj =
        instr->FindMirroredObjectByOperand(view->src().operand(), stream.parallel_id());
    CHECK_NOTNULL(src_mirrored_obj);
    src = src_mirrored_obj->mut_cuda_mem_buffer()->mut_data();
  }
  Memcpy<DeviceType::kGPU>(stream.device_ctx().get(), dst, src, size,
                           cudaMemcpyKind::cudaMemcpyDeviceToHost);
}

}  // namespace

void CudaCopyD2HStreamType::InitDeviceCtx(std::unique_ptr<DeviceCtx>* device_ctx,
                                          Stream* stream) const {
  device_ctx->reset(new CudaStreamHandleDeviceCtx(stream->mut_callback_list()));
}

void CudaCopyD2HStreamType::InitInstructionStatus(const Stream& stream,
                                                  InstructionStatusBuffer* status_buffer) const {
  static_assert(sizeof(CudaInstrStatusQuerier) < kInstructionStatusBufferBytes, "");
  CudaInstrStatusQuerier::PlacementNew(status_buffer->mut_buffer()->mut_data(),
                                       stream.thread_ctx().device_id());
}

void CudaCopyD2HStreamType::DeleteInstructionStatus(const Stream& stream,
                                                    InstructionStatusBuffer* status_buffer) const {
  // do nothing
}

bool CudaCopyD2HStreamType::QueryInstructionStatusDone(
    const Stream& stream, const InstructionStatusBuffer& status_buffer) const {
  return CudaInstrStatusQuerier::Cast(status_buffer.buffer().data())->done();
}

void CudaCopyD2HStreamType::Compute(InstrChain* instr_chain) const {
  auto* stream = instr_chain->mut_stream();
  cudaSetDevice(stream->thread_ctx().device_id());
  OBJECT_MSG_LIST_UNSAFE_FOR_EACH_PTR(instr_chain->mut_instruction_list(), instruction) {
    CudaCopyD2H(instruction);
  }
  stream->mut_callback_list()->MoveTo(instr_chain->mut_callback_list());
  char* data_ptr = instr_chain->mut_status_buffer()->mut_buffer()->mut_data();
  CudaInstrStatusQuerier::MutCast(data_ptr)->SetLaunched(stream->device_ctx().get());
}

ObjectMsgPtr<StreamDesc> CudaCopyD2HStreamType::MakeRemoteStreamDesc(
    const Resource& resource, int64_t this_machine_id) const {
  std::size_t device_num = resource.gpu_device_num();
  auto ret = ObjectMsgPtr<StreamDesc>::New();
  ret->mutable_stream_type_id()->__Init__(typeid(CudaCopyD2HStreamType));
  ret->set_num_machines(1);
  ret->set_num_streams_per_machine(device_num);
  ret->set_num_streams_per_thread(1);
  ret->set_start_parallel_id(this_machine_id * device_num);
  return ret;
}

COMMAND(RegisterStreamType<CudaCopyD2HStreamType>());
COMMAND(RegisterInstrTypeId<CudaCopyD2HStreamType>("CopyD2H", 0, kRemote));
COMMAND(RegisterInstrTypeId<CudaCopyD2HStreamType>("CudaCopyD2H", 0, kRemote));

}  // namespace vm
}  // namespace oneflow