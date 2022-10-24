#include "asmjs/shared-constants.h"
#include "shared-constants.h"
#include <pass.h>
#include <wasm-builder.h>
#include <wasm.h>
#include "ir/names.h"

namespace wasm{

const int i = 1;
#define is_bigendian() ( (*(char*)&i) == 0 )

struct Canonicalize : public WalkerPass<ControlFlowWalker<Canonicalize, UnifiedExpressionVisitor<Canonicalize>>> {

  Name canon32, canon64;

  void visitStore(Store* curr) {
    Builder builder(*getModule());
    bool replacement = false;
    if (curr->value->type == Type::f32) {
      curr->value = builder.makeCall(canon32, {curr->value}, Type::f32);
      replacement = true;
    } else if (curr->value->type == Type::f64) {
      curr->value = builder.makeCall(canon64, {curr->value}, Type::f64);
      replacement = true;
    }
    if (replacement) {
      if (getFunction()) {
        replaceCurrent(curr);
      } else {
        std::cout << "warning: cannot de-nan outside of function context\n";
      }
    }
  }

  void doWalkModule(Module* module) {
    float canon_f;
    double canon_d;
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

    if (is_bigendian()) {
      unsigned char float_bytes[] = {0x7f, 0xc0, 0x00, 0x00};
      unsigned char double_bytes[] = {0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
      memcpy(&canon_f, float_bytes, sizeof(float));
      memcpy(&canon_d, double_bytes, sizeof(double));
    } else {
      unsigned char float_bytes[] = {0x00, 0x00, 0xc0, 0x7f};
      unsigned char double_bytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f};
      memcpy(&canon_f, float_bytes, sizeof(float));
      memcpy(&canon_d, double_bytes, sizeof(double));
    }
    add(canon32, Type::f32, Literal(canon_f), EqFloat32);
    add(canon64, Type::f64, Literal(canon_d), EqFloat64);
  }

};

Pass* createCanonicalizePass() { return new Canonicalize(); }

}