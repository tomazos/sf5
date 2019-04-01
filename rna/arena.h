#pragma once

namespace rna {

class Arena {
  std::unordered_map<uint16_t, Object> objects_;
};

}  // namespace rna
