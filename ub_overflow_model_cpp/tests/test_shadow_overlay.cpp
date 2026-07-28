#include "../src/ir/shadow_overlay.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

cvub::GenericModule makeModule() {
  cvub::GenericModule module;
  module.operations.resize(4);
  module.regions.resize(2);
  module.blocks.resize(2);

  module.operations[0].id = 0;
  module.operations[0].name = "builtin.module";
  module.operations[0].regions = {0};
  module.operations[1].id = 1;
  module.operations[1].parentId = 0;
  module.operations[1].regionId = 0;
  module.operations[1].blockId = 0;
  module.operations[1].name = "func.func";
  module.operations[1].regions = {1};
  module.operations[2].id = 2;
  module.operations[2].parentId = 1;
  module.operations[2].regionId = 1;
  module.operations[2].blockId = 1;
  module.operations[2].name = "arith.constant";
  module.operations[2].results = {0};
  module.operations[2].resultTypes = {"i32"};
  module.operations[2].attributes = "{value = 1 : i32}";
  module.operations[3].id = 3;
  module.operations[3].parentId = 1;
  module.operations[3].regionId = 1;
  module.operations[3].blockId = 1;
  module.operations[3].name = "test.consume";
  module.operations[3].operands = {0};
  module.operations[3].operandTypes = {"i32"};
  module.operations[3].dpsInputs = {0};

  module.regions[0].id = 0;
  module.regions[0].parentOperation = 0;
  module.regions[0].blocks = {0};
  module.regions[1].id = 1;
  module.regions[1].parentOperation = 1;
  module.regions[1].blocks = {1};
  module.blocks[0].id = 0;
  module.blocks[0].regionId = 0;
  module.blocks[0].operations = {1};
  module.blocks[1].id = 1;
  module.blocks[1].regionId = 1;
  module.blocks[1].operations = {2, 3};
  return module;
}

void testMutationPrimitives() {
  cvub::GenericModule module = makeModule();
  cvub::GenericShadowOverlay overlay(module);
  const cvub::BlockId body = cvub::BlockId::fromIndex(1);
  const cvub::OpId constant = cvub::OpId::fromIndex(2);
  const cvub::OpId consumer = cvub::OpId::fromIndex(3);
  const cvub::ValueId oldValue = cvub::ValueId::fromIndex(0);

  const cvub::OpId synthetic = overlay.createOperation(
      body, "arith.constant", {"i32"});
  require(synthetic.raw() == 4, "synthetic IDs must append after base IDs");
  require(overlay.block(body).operations.back() == synthetic,
          "synthetic operation order must be explicit");

  overlay.replaceAllUses(oldValue, cvub::ValueId::fromIndex(1));
  require(overlay.operands(consumer).front().raw() == 1,
          "replaceAllUses must update the operand overlay");
  require(overlay.dpsInputs(consumer).front().raw() == 1,
          "replaceAllUses must update semantic DPS uses");
  require(overlay.users(oldValue).empty(),
          "replaceAllUses must update the delta use-list");

  overlay.moveToBlock(synthetic, body, 1);
  require(overlay.block(body).operations[1] == synthetic,
          "move must preserve requested block order");
  overlay.eraseOperation(constant);
  require(!overlay.isActive(constant), "erase must leave a tombstone");
  require(overlay.operationArenaSize() == 5,
          "erase must not recycle or renumber stable IDs");

  overlay.setAttributes(consumer, "{tag = true}");
  overlay.setEffects(consumer, "read@0");
  require(overlay.attributes(consumer) == "{tag = true}" &&
              overlay.effects(consumer) == "read@0",
          "typed field overrides must be isolated in the overlay");
  require(module.operations[3].attributes.empty(),
          "overlay must not mutate its immutable base module");

  cvub::GenericModule materialized =
      overlay.materializeLegacyGenericModule();
  require(materialized.operations.size() == 4,
          "materialization must remove tombstones at one boundary");
  const cvub::GenericOperation *materializedConsumer = nullptr;
  const cvub::GenericOperation *materializedConstant = nullptr;
  for (const cvub::GenericOperation &operation : materialized.operations)
    if (operation.name == "test.consume")
      materializedConsumer = &operation;
    else if (operation.name == "arith.constant")
      materializedConstant = &operation;
  require(materializedConsumer &&
              materializedConstant &&
              materializedConsumer->operands.front() ==
                  materializedConstant->results.front() &&
              materializedConsumer->dpsInputs.front() ==
                  materializedConstant->results.front() &&
              materializedConsumer->attributes == "{tag = true}",
          "materialization must preserve structural and field overrides");
}

void testNestedAndCloneSemantics() {
  cvub::GenericModule module = makeModule();
  cvub::GenericShadowOverlay overlay(module);
  const cvub::BlockId body = cvub::BlockId::fromIndex(1);
  const cvub::OpId consumer = cvub::OpId::fromIndex(3);
  const cvub::OpId clone = overlay.cloneSemanticNode(consumer, body);
  require(overlay.operation(clone).projectionSource == consumer,
          "clone must retain source identity");

  const cvub::RegionId nested = overlay.createRegion(clone);
  const cvub::BlockId nestedBlock = overlay.createBlock(nested, {"i32"});
  const cvub::OpId nestedOperation =
      overlay.createOperation(nestedBlock, "test.nested", {});
  overlay.eraseOperationTree(clone);
  require(!overlay.isActive(clone) && !overlay.isActive(nestedOperation),
          "tree erase must tombstone nested synthetic operations");
  require(!overlay.region(nested).active && !overlay.block(nestedBlock).active,
          "tree erase must tombstone nested regions and blocks");
}

void testReplaceUsesExcept() {
  cvub::GenericModule module = makeModule();
  cvub::GenericShadowOverlay overlay(module);
  const cvub::BlockId body = cvub::BlockId::fromIndex(1);
  const cvub::OpId first = cvub::OpId::fromIndex(3);
  const cvub::OpId second = overlay.createOperation(
      body, "test.consume", {}, {cvub::ValueId::fromIndex(0)});
  overlay.replaceUsesExcept(cvub::ValueId::fromIndex(0),
                            cvub::ValueId::fromIndex(1), {first});
  require(overlay.operands(first).front().raw() == 0 &&
              overlay.operands(second).front().raw() == 1,
          "replaceUsesExcept must preserve excluded users");
}

void testLazyCopyOnWritePayload() {
  int materializations = 0;
  cvub::CowString original = cvub::CowString::Deferred([&] {
    ++materializations;
    return std::string("{value = 1 : i32}");
  });
  cvub::CowString projection = original;
  require(materializations == 0,
          "copying a base payload must not materialize MLIR text");
  require(projection.find("value") != std::string::npos &&
              materializations == 1,
          "shared deferred payload must materialize exactly once");
  projection.replace(9, 1, "2");
  require(original.find("1 : i32") != std::string::npos &&
              projection.find("2 : i32") != std::string::npos,
          "mutating a projection must detach from the base payload");
}

} // namespace

int main() {
  testMutationPrimitives();
  testNestedAndCloneSemantics();
  testReplaceUsesExcept();
  testLazyCopyOnWritePayload();
  std::cout << "[PASS] stable-ID shadow overlay mutation primitives\n";
  return 0;
}
