#pragma once

#include <memory>
#include <unordered_map>

#include "dvc/container.h"
#include "rna/object.h"

namespace rna {

using ObjectId = uint64_t;

class Arena {
 public:
  ObjectId add_object(std::unique_ptr<Object> object) {
    ObjectId id = next_id++;
    dvc::insert_or_die(object_map, id, std::move(object));
    return id;
  }

  void remove_object(ObjectId id) { object_map.erase(id); }

 private:
  ObjectId next_id = 0;
  std::unordered_map<ObjectId, std::unique_ptr<Object>> object_map;
};

}  // namespace rna
