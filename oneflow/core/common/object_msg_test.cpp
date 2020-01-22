#define private public
#include "oneflow/core/common/util.h"
#include "oneflow/core/common/object_msg.h"
#include "oneflow/core/common/preprocessor.h"

namespace oneflow {

namespace test {

TEST(ObjectMsgStruct, ref_cnt) {
  class Foo final : public ObjectMsgStruct {
   public:
    Foo() = default;
  };
  Foo foo;
  foo.InitRefCount();
  foo.IncreaseRefCount();
  foo.IncreaseRefCount();
  ASSERT_EQ(foo.DecreaseRefCount(), 1);
  ASSERT_EQ(foo.DecreaseRefCount(), 0);
}

class TestNew final : public ObjectMsgStruct {
  BEGIN_DSS(DSS_GET_DEFINE_COUNTER(), TestNew, sizeof(ObjectMsgStruct));

  END_DSS(DSS_GET_DEFINE_COUNTER(), "object_msg", TestNew);
};

TEST(ObjectMsgPtr, obj_new) { ObjectMsgPtr<TestNew>::New(); }

// clang-format off
BEGIN_OBJECT_MSG(ObjectMsgFoo)
 public:
  void __Delete__();

  OBJECT_MSG_DEFINE_OPTIONAL(int8_t, x);
  OBJECT_MSG_DEFINE_OPTIONAL(int32_t, foo);
  OBJECT_MSG_DEFINE_OPTIONAL(int16_t, bar);
  OBJECT_MSG_DEFINE_OPTIONAL(int64_t, foobar);
  OBJECT_MSG_DEFINE_RAW_PTR(std::string*, is_deleted);
END_OBJECT_MSG(ObjectMsgFoo)
// clang-format on

void OBJECT_MSG_TYPE(ObjectMsgFoo)::__Delete__() {
  if (mutable_is_deleted()) { *mutable_is_deleted() = "deleted"; }
}

TEST(OBJECT_MSG, naive) {
  auto foo = OBJECT_MSG_PTR(ObjectMsgFoo)::New();
  foo->set_bar(9527);
  ASSERT_TRUE(foo->bar() == 9527);
}

TEST(OBJECT_MSG, __delete__) {
  std::string is_deleted;
  {
    auto foo = OBJECT_MSG_PTR(ObjectMsgFoo)::New();
    foo->set_bar(9527);
    foo->set_is_deleted(&is_deleted);
    ASSERT_EQ(foo->bar(), 9527);
  }
  ASSERT_TRUE(is_deleted == "deleted");
}

// clang-format off
BEGIN_OBJECT_MSG(ObjectMsgBar)
 public:
  void __Delete__(){
    if (mutable_is_deleted()) { *mutable_is_deleted() = "bar_deleted"; }
  }
  OBJECT_MSG_DEFINE_OPTIONAL(ObjectMsgFoo, foo);
  OBJECT_MSG_DEFINE_RAW_PTR(std::string*, is_deleted);
END_OBJECT_MSG(ObjectMsgBar)
// clang-format on

TEST(OBJECT_MSG, nested_objects) {
  auto bar = OBJECT_MSG_PTR(ObjectMsgBar)::New();
  bar->mutable_foo()->set_bar(9527);
  ASSERT_TRUE(bar->foo().bar() == 9527);
}

TEST(OBJECT_MSG, nested_delete) {
  std::string bar_is_deleted;
  std::string is_deleted;
  {
    auto bar = OBJECT_MSG_PTR(ObjectMsgBar)::New();
    bar->set_is_deleted(&bar_is_deleted);
    auto* foo = bar->mutable_foo();
    foo->set_bar(9527);
    foo->set_is_deleted(&is_deleted);
    ASSERT_EQ(foo->bar(), 9527);
  }
  ASSERT_EQ(is_deleted, std::string("deleted"));
  ASSERT_EQ(bar_is_deleted, std::string("bar_deleted"));
}

// clang-format off
BEGIN_OBJECT_MSG(TestScalarOneof)
  OBJECT_MSG_DEFINE_ONEOF(type,
      OBJECT_MSG_ONEOF_FIELD(int32_t, x)
      OBJECT_MSG_ONEOF_FIELD(int64_t, foo));
END_OBJECT_MSG(TestScalarOneof)
// clang-format on

// clang-format off
BEGIN_OBJECT_MSG(TestPtrOneof)
  OBJECT_MSG_DEFINE_ONEOF(type,
      OBJECT_MSG_ONEOF_FIELD(ObjectMsgFoo, foo)
      OBJECT_MSG_ONEOF_FIELD(int32_t, int_field));
END_OBJECT_MSG(TestPtrOneof)
// clang-format on

TEST(OBJECT_MSG, oneof_get) {
  auto test_oneof = OBJECT_MSG_PTR(TestPtrOneof)::New();
  auto& obj = *test_oneof;
  const auto* default_foo_ptr = &obj.foo();
  ASSERT_EQ(obj.foo().x(), 0);
  ASSERT_TRUE(!obj.has_foo());
  obj.mutable_foo();
  ASSERT_TRUE(obj.has_foo());
  ASSERT_EQ(obj.foo().x(), 0);
  ASSERT_NE(default_foo_ptr, &obj.foo());
};

TEST(OBJECT_MSG, oneof_release) {
  auto test_oneof = OBJECT_MSG_PTR(TestPtrOneof)::New();
  auto& obj = *test_oneof;
  const auto* default_foo_ptr = &obj.foo();
  ASSERT_EQ(obj.foo().x(), 0);
  obj.mutable_foo();
  ASSERT_EQ(obj.foo().x(), 0);
  ASSERT_NE(default_foo_ptr, &obj.foo());
  {
    std::string is_delete;
    obj.mutable_foo()->set_is_deleted(&is_delete);
    obj.mutable_int_field();
    ASSERT_EQ(is_delete, "deleted");
  }
  {
    std::string is_delete;
    obj.mutable_foo()->set_is_deleted(&is_delete);
    obj.mutable_int_field();
    ASSERT_EQ(is_delete, "deleted");
  }
};

TEST(OBJECT_MSG, oneof_clear) {
  auto test_oneof = OBJECT_MSG_PTR(TestPtrOneof)::New();
  auto& obj = *test_oneof;
  const auto* default_foo_ptr = &obj.foo();
  ASSERT_EQ(obj.foo().x(), 0);
  obj.mutable_foo();
  ASSERT_EQ(obj.foo().x(), 0);
  ASSERT_NE(default_foo_ptr, &obj.foo());
  {
    std::string is_delete;
    obj.mutable_foo()->set_is_deleted(&is_delete);
    ASSERT_TRUE(!obj.has_int_field());
    obj.clear_int_field();
    ASSERT_TRUE(!obj.has_int_field());
    ASSERT_TRUE(obj.has_foo());
    obj.clear_foo();
    ASSERT_TRUE(!obj.has_foo());
    ASSERT_EQ(is_delete, "deleted");
  }
};

TEST(OBJECT_MSG, oneof_set) {
  auto test_oneof = OBJECT_MSG_PTR(TestPtrOneof)::New();
  auto& obj = *test_oneof;
  const auto* default_foo_ptr = &obj.foo();
  ASSERT_EQ(obj.foo().x(), 0);
  obj.mutable_foo();
  ASSERT_EQ(obj.foo().x(), 0);
  ASSERT_NE(default_foo_ptr, &obj.foo());
  {
    std::string is_delete;
    obj.mutable_foo()->set_is_deleted(&is_delete);
    ASSERT_TRUE(!obj.has_int_field());
    obj.clear_int_field();
    ASSERT_TRUE(!obj.has_int_field());
    ASSERT_TRUE(obj.has_foo());
    obj.set_int_field(30);
    ASSERT_TRUE(!obj.has_foo());
    ASSERT_EQ(is_delete, "deleted");
  }
};

// clang-format off
BEGIN_FLAT_MSG(FlatMsgDemo)
  FLAT_MSG_DEFINE_ONEOF(type,
      FLAT_MSG_ONEOF_FIELD(int32_t, int32_field)
      FLAT_MSG_ONEOF_FIELD(float, float_field));
END_FLAT_MSG(FlatMsgDemo)
// clang-format on

// clang-format off
BEGIN_OBJECT_MSG(ObjectMsgContainerDemo)
  OBJECT_MSG_DEFINE_FLAT_MSG(FlatMsgDemo, flat_field);
END_OBJECT_MSG(ObjectMsgContainerDemo)
// clang-format on

TEST(OBJECT_MSG, flat_msg_field) {
  auto obj = OBJECT_MSG_PTR(ObjectMsgContainerDemo)::New();
  ASSERT_TRUE(obj->has_flat_field());
  ASSERT_TRUE(!obj->flat_field().has_int32_field());
  obj->mutable_flat_field()->set_int32_field(33);
  ASSERT_TRUE(obj->flat_field().has_int32_field());
  ASSERT_EQ(obj->flat_field().int32_field(), 33);
}

}  // namespace test

}  // namespace oneflow