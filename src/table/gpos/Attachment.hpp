#pragma once

namespace eglyf::gpos {

struct Attachment {
  std::shared_ptr<Anchor> receptor;
  std::shared_ptr<Anchor> ligand;
};

} // namespace eglyf::gpos
