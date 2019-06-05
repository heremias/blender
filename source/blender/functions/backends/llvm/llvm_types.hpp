#pragma once

#include "FN_core.hpp"

#include "builder.hpp"

#include <functional>
#include <mutex>

namespace FN {

class LLVMTypeInfo : public TypeExtension {
 public:
  BLI_COMPOSITION_DECLARATION(LLVMTypeInfo);

  virtual ~LLVMTypeInfo();

  virtual llvm::Type *get_type(llvm::LLVMContext &context) const = 0;
  virtual llvm::Value *build_copy_ir(CodeBuilder &builder, llvm::Value *value) const = 0;
  virtual void build_free_ir(CodeBuilder &builder, llvm::Value *value) const = 0;
  virtual void build_store_ir__relocate(CodeBuilder &builder,
                                        llvm::Value *value,
                                        llvm::Value *address) const = 0;
  virtual void build_store_ir__copy(CodeBuilder &builder,
                                    llvm::Value *value,
                                    llvm::Value *address) const = 0;
  virtual llvm::Value *build_load_ir__copy(CodeBuilder &builder, llvm::Value *address) const = 0;
  virtual llvm::Value *build_load_ir__relocate(CodeBuilder &builder,
                                               llvm::Value *address) const = 0;
};

/* Trivial: The type could be copied with memcpy
 *   and freeing the type does nothing.
 *   Subclasses still have to implement functions to
 *   store and load this type from memory. */
class TrivialLLVMTypeInfo : public LLVMTypeInfo {
 public:
  llvm::Value *build_copy_ir(CodeBuilder &builder, llvm::Value *value) const override;
  void build_free_ir(CodeBuilder &builder, llvm::Value *value) const override;
  void build_store_ir__relocate(CodeBuilder &builder,
                                llvm::Value *value,
                                llvm::Value *address) const override;
  llvm::Value *build_load_ir__relocate(CodeBuilder &builder, llvm::Value *address) const override;
};

/* Packed: The memory layout in llvm matches
 *   the layout used in the rest of the C/C++ code.
 *   That means, that no special load/store functions
 *   have to be written. */
class PackedLLVMTypeInfo : public TrivialLLVMTypeInfo {
 private:
  typedef std::function<llvm::Type *(llvm::LLVMContext &context)> CreateFunc;
  CreateFunc m_create_func;

 public:
  PackedLLVMTypeInfo(CreateFunc create_func) : m_create_func(create_func)
  {
  }

  llvm::Type *get_type(llvm::LLVMContext &context) const override;
  llvm::Value *build_load_ir__copy(CodeBuilder &builder, llvm::Value *address) const override;
  void build_store_ir__copy(CodeBuilder &builder,
                            llvm::Value *value,
                            llvm::Value *address) const override;
};

class PointerLLVMTypeInfo : public LLVMTypeInfo {
 private:
  typedef std::function<void *(void *)> CopyFunc;
  typedef std::function<void(void *)> FreeFunc;
  typedef std::function<void *()> DefaultFunc;

  CopyFunc m_copy_func;
  FreeFunc m_free_func;
  DefaultFunc m_default_func;

  static void *copy_value(PointerLLVMTypeInfo *info, void *value);
  static void free_value(PointerLLVMTypeInfo *info, void *value);
  static void *default_value(PointerLLVMTypeInfo *info);

 public:
  PointerLLVMTypeInfo(CopyFunc copy_func, FreeFunc free_func, DefaultFunc default_func)
      : m_copy_func(copy_func), m_free_func(free_func), m_default_func(default_func)
  {
  }

  llvm::Type *get_type(llvm::LLVMContext &context) const override;
  llvm::Value *build_copy_ir(CodeBuilder &builder, llvm::Value *value) const override;
  void build_free_ir(CodeBuilder &builder, llvm::Value *value) const override;
  void build_store_ir__copy(CodeBuilder &builder,
                            llvm::Value *value,
                            llvm::Value *address) const override;
  void build_store_ir__relocate(CodeBuilder &builder,
                                llvm::Value *value,
                                llvm::Value *address) const override;
  llvm::Value *build_load_ir__copy(CodeBuilder &builder, llvm::Value *address) const override;
  llvm::Value *build_load_ir__relocate(CodeBuilder &builder, llvm::Value *address) const override;
};

inline LLVMTypeInfo *get_type_info(const SharedType &type)
{
  auto ext = type->extension<LLVMTypeInfo>();
  BLI_assert(ext);
  return ext;
}

inline llvm::Type *get_llvm_type(SharedType &type, llvm::LLVMContext &context)
{
  return get_type_info(type)->get_type(context);
}

LLVMTypes types_of_type_infos(const SmallVector<LLVMTypeInfo *> &type_infos,
                              llvm::LLVMContext &context);

} /* namespace FN */