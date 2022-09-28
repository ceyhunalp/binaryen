#include "asmjs/shared-constants.h"
#include "shared-constants.h"
#include <pass.h>
#include <wasm-builder.h>
#include <wasm.h>
#include "ir/names.h"

namespace wasm{

struct Canonicalize : public WalkerPass<ControlFlowWalker<Canonicalize, UnifiedExpressionVisitor<Canonicalize>>> {

  Name canon32, canon64;

  void visitStore(Store* curr) {
    curr->dump();
    Builder builder(*getModule());
//    Expression* replacement = nullptr;
    bool replacement = false;
    if (curr->value->type == Type::f32) {
      curr->value = builder.makeCall(canon32, {curr->value}, Type::f32);
      replacement = true;
//      replacement = builder.makeCall(canon_store_f32, {curr}, curr->value->type);
    } else if (curr->value->type == Type::f64) {
      curr->value = builder.makeCall(canon64, {curr->value}, Type::f64);
      replacement = true;
//      replacement = builder.makeCall(canon_store_f64, {curr}, curr->value->type);
    }
    if (replacement) {
      std::cout << "Replacement (after)" << std::endl;
      curr->dump();
      if (getFunction()) {
        replaceCurrent(curr);
      } else {
        std::cout << "warning: cannot de-nan outside of function context\n";
      }
    }
    curr->dump();
  }

  void doWalkModule(Module* module) {
    // Pick names for the helper functions.
    canon32 = Names::getValidFunctionName(*module, "canon32");
    canon64 = Names::getValidFunctionName(*module, "canon64");

    ControlFlowWalker<Canonicalize, UnifiedExpressionVisitor<Canonicalize>>::doWalkModule(
      module);

    // Add helper functions after the walk, so they are not instrumented.
    Builder builder(*module);
    auto add = [&](Name name, Type type, Literal literal, BinaryOp op) {
      auto func = Builder::makeFunction(name, Signature(type, type), {});
      // Compare the value to itself to check if it is a NaN, and return 0 if
      // so:
      //
      //   (if (result f*)
      //     (f*.eq
      //       (local.get $0)
      //       (local.get $0)
      //     )
      //     (local.get $0)
      //     (f*.const 0)
      //   )
      func->body = builder.makeIf(
        builder.makeBinary(
          op, builder.makeLocalGet(0, type), builder.makeLocalGet(0, type)),
        builder.makeLocalGet(0, type),
        builder.makeConst(literal));
      module->addFunction(std::move(func));
    };
    add(canon32, Type::f32, Literal(float(0)), EqFloat32);
    add(canon64, Type::f64, Literal(double(0)), EqFloat64);
  }

//  void visitStore(Store* curr) {
////    Builder builder(*getModule());
////    printf("value type\n");
////    std::cout << curr->valueType.toString();
////    std::cout << "=====";
//    switch (curr->value->type.getBasic()) {
////      case Type::f32:
////        printf("Type is f32\n");
////        break;
//      case Type::f64: {
////        curr->dump();
////        printf("PTR DUMP\n");
////        curr->ptr->dump();
////        printf("VALUE DUMP\n");
////        curr->value->dump();
//        auto* c = curr->value->dynCast<LocalGet>();
//        if (c) {
//          seenIndexes.insert(c->index);
//          printf("Inserting index: %d\n", c->index);
//          std::cout << "----" << std::endl;
////          printf("Current expression:\n");
//        }
//        break;}
//      default:
//        return;
//    }
//  }

//private:
//  void addImport(Module* curr, Name name, Type params, Type results) {
//    auto import = Builder::makeFunction(name, Signature(params, results), {});
//    import->module = ENV;
//    import->base = name;
//    curr->addFunction(std::move(import));
//  }
};

Pass* createCanonicalizePass() { return new Canonicalize(); }

}