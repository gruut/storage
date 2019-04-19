#ifndef GRUUT_PUBLIC_MERGER_UNRESOLVED_BLOCK_POOL_HPP
#define GRUUT_PUBLIC_MERGER_UNRESOLVED_BLOCK_POOL_HPP

#include <atomic>
#include <deque>

#include "../chain/types.hpp"
#include "../utils/type_converter.hpp"

#include "../chain/block.hpp"
#include "../chain/block_validator.hpp"
#include "../chain/state_merkle_tree.hpp"
#include "storage_manager.hpp"

namespace gruut {

struct UnresolvedBlock {
  Block block;
  int prev_vector_idx{-1};

  UnresolvedBlock() = default;
  UnresolvedBlock(Block &block_, int prev_queue_idx_) : block(block_), prev_vector_idx(prev_queue_idx_) {}
};

struct BlockPosPool {
  size_t height{0};
  size_t vector_idx{0};

  BlockPosPool() = default;
  BlockPosPool(size_t height_, size_t vector_idx_) : height(height_), vector_idx(vector_idx_) {}
};

class UnresolvedBlockPool : public TemplateSingleton<UnresolvedBlockPool> {
private:
  std::deque<std::vector<UnresolvedBlock>> m_block_pool; // deque[n] is tree's depth n; vector's blocks are same depth(block height)
  std::recursive_mutex m_push_mutex;

  StateMerkleTree m_us_tree; // user scope state tree
  StateMerkleTree m_cs_tree; // contract scope state tree

  base64_type m_latest_confirmed_id;
  std::atomic<block_height_type> m_latest_confirmed_height;
  timestamp_t m_latest_confirmed_time;
  base64_type m_latest_confirmed_hash;
  base64_type m_latest_confirmed_prev_id;

  base64_type m_head_id;
  std::atomic<block_height_type> m_head_height;

  StorageManager *m_storage_manager;

public:
  UnresolvedBlockPool();

  inline size_t size() {
    return m_block_pool.size();
  }

  inline bool empty() {
    return m_block_pool.empty();
  }

  inline void clear() {
    m_block_pool.clear();
  }

  void setPool(const base64_type &last_block_id, block_height_type last_height, timestamp_t last_time, const base64_type &last_hash,
               const base64_type &prev_block_id);

  bool prepareBins(block_height_type t_height);
  unblk_push_result_type push(Block &block, bool is_restore = false);

  void getResolvedBlocks(std::vector<UnresolvedBlock> &resolved_blocks, std::vector<std::string> &drop_blocks);

  nth_link_type getMostPossibleLink();

  void restorePool();
  void setupStateTree();

  void process_tx_result(Block &new_block, nlohmann::json &result);
  void move_head(const std::string &block_id_b64, const block_height_type target_block_height);

  bool hasUnresolvedBlocks();
  void invalidateCaches();
  bool getBlock(block_height_type t_height, const hash_t &t_prev_hash, const hash_t &t_hash, Block &ret_block);
  bool getBlock(block_height_type t_height, Block &ret_block);
  nth_link_type getUnresolvedLowestLink();

private:
  nlohmann::json readBackupIds();
  void backupPool();

  BlockPosPool getLongestBlockPos();
  void updateConfirmLevel();
};

} // namespace gruut

#endif // WORKSPACE_UNRESOLVED_BLOCK_POOL_HPP