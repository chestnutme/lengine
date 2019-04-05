/**
 * b_plus_tree_internal_page.cpp
 */
#include <iostream>
#include <sstream>

#include "common/exception.h"
#include "page/b_plus_tree_internal_page.h"

namespace cmudb {
/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/
/*
 * Init method after creating a new internal page
 * Including set page type, set current size, set page id, set parent id and set
 * max page size
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
Init(page_id_t page_id, page_id_t parent_id) {
  // set page type
  SetPageType(IndexPageType::INTERNAL_PAGE);
  // set current size: 1 for the first invalid key
  SetSize(1);
  // set page id
  SetPageId(page_id);
  // set parent id
  SetParentPageId(parent_id);

  // set max page size, header is 24bytes
  int size = (PAGE_SIZE - sizeof(BPlusTreeInternalPage))/
      (sizeof(KeyType) + sizeof(ValueType));
  SetMaxSize(size);
}

/*
 * Helper method to get/set the key associated with input "index"(a.k.a
 * array offset)
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
KeyType BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
KeyAt(int index) const {
  // replace with your own code
  assert(0 <= index && index < GetSize());
  return array[index].first;
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
SetKeyAt(int index, const KeyType &key) {
  assert(0 <= index && index < GetSize());
  array[index].first = key;
}

/*
 * Helper method to find and return array index(or offset), so that its value
 * equals to input "value"
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
int BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
ValueIndex(const ValueType &value) const {
  for (int i = 0; i < GetSize(); ++i) {
    if (array[i].second == value) {
      return i;
    }
  }
  return GetSize();
}

/*
 * Helper method to get the value associated with input "index"(a.k.a array
 * offset)
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
ValueType BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
ValueAt(int index) const {
  assert(0 <= index && index < GetSize());
  return array[index].second;
}

/*****************************************************************************
 * LOOKUP
 *****************************************************************************/
/*
 * Find and return the child pointer(page_id) which points to the child page
 * that contains input "key"
 * Start the search from the second key(the first key should always be invalid)
 *
 * //sequential scan
 * ValueType dir = array[0].second;
 * for (int i = 1; i < GetSize(); ++i) {
 *   if (comparator(key, array[i].first) < 0) {
 *     break;
 *   }
 *   dir = array[i].second;
 * }
 * return dir;
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
ValueType BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
Lookup(const KeyType &key, const KeyComparator &comparator) const {
  // binary search
  assert(GetSize() > 1);
  if (comparator(key, array[1].first) < 0) {
    return array[0].second;
  } else if (comparator(key, array[GetSize() - 1].first) >= 0) {
    return array[GetSize() - 1].second;
  }

  int low = 1, high = GetSize() - 1, mid;
  while (low < high && low + 1 != high) {
    mid = low + (high - low)/2;
    if (comparator(key, array[mid].first) < 0) {
      high = mid;
    } else if (comparator(key, array[mid].first) > 0) {
      low = mid;
    } else {
      return array[mid].second;
    }
  }
  return array[low].second;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Populate new root page with old_value + new_key & new_value
 * When the insertion cause overflow from leaf page all the way upto the root
 * page, you should create a new root page and populate its elements.
 * NOTE: This method is only called within InsertIntoParent()(b_plus_tree.cpp)
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
PopulateNewRoot(const ValueType &old_value, const KeyType &new_key,
                const ValueType &new_value) {
  // must be an empty page
  assert(GetSize() == 1);
  array[0].second = old_value;
  array[1] = {new_key, new_value};
  IncreaseSize(1);
}

/*
 * Insert new_key & new_value pair right after the pair with its value ==
 * old_value
 * @return:  new size after insertion
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
int BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
InsertNodeAfter(const ValueType &old_value, const KeyType &new_key,
                const ValueType &new_value) {
  for (int i = GetSize(); i > 0; --i) {
    if (array[i - 1].second == old_value) {
      array[i] = {new_key, new_value};
      IncreaseSize(1);
      break;
    }
    array[i] = array[i - 1];
  }
  return GetSize();
}

/*****************************************************************************
 * SPLIT
 *****************************************************************************/
/*
 * Remove half of key & value pairs from this page to "recipient" page
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
MoveHalfTo(BPlusTreeInternalPage *recipient,
           BufferPoolManager *buffer_pool_manager) {
  auto half = GetSize()/2;
  recipient->CopyHalfFrom(array + GetSize() - half, half, buffer_pool_manager);
  IncreaseSize(-1*half);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
CopyHalfFrom(MappingType *items, int size,
             BufferPoolManager *buffer_pool_manager) {
  // must be a new page
  assert(!IsLeafPage() && GetSize() == 1);
  for (int i = 0; i < size; ++i) {
    array[i] = *items++;
  }
  IncreaseSize(size);
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Remove the key & value pair in internal page according to input index(a.k.a
 * array offset)
 * NOTE: store key&value pair continuously after deletion
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
Remove(int index) {
  assert(0 <= index && index < GetSize());
  for (int i = index; i < GetSize() - 1; ++i) {
    array[index] = array[index + 1];
  }
  IncreaseSize(-1);
}

/*
 * Remove the only key & value pair in internal page and return the value
 * NOTE: only call this method within AdjustRoot()(in b_plus_tree.cpp)
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
ValueType BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
RemoveAndReturnOnlyChild() {
  IncreaseSize(-1);
  assert(GetSize() == 1);
  return ValueAt(0);
}

/*****************************************************************************
 * MERGE
 *****************************************************************************/
/*
 * Remove all of key & value pairs from this page to "recipient" page, then
 * update relevant key & value pair in its parent page.
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
MoveAllTo(BPlusTreeInternalPage *recipient, int index_in_parent,
          BufferPoolManager *buffer_pool_manager) {
  // assumption: current page is at the right hand of recipient
  recipient->CopyAllFrom(array + 1, GetSize() - 1, buffer_pool_manager);

  auto *parent = reinterpret_cast<BPlusTreeInternalPage *>(
      buffer_pool_manager->FetchPage(GetParentPageId()));
  parent->Remove(index_in_parent);

  // unpin parent page
  buffer_pool_manager->UnpinPage(parent->GetPageId(), true);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
CopyAllFrom(MappingType *items, int size,
            BufferPoolManager *buffer_pool_manager) {
  assert(GetSize() + size < GetMaxSize());
  for (int i = GetSize(); i < size; ++i) {
    array[i] = *items++;
  }
  IncreaseSize(size);
}

/*****************************************************************************
 * REDISTRIBUTE
 *****************************************************************************/
/*
 * Remove the first key & value pair from this page to tail of "recipient"
 * page, then update relevant key & value pair in its parent page.
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
MoveFirstToEndOf(BPlusTreeInternalPage *recipient,
                 BufferPoolManager *buffer_pool_manager) {
  assert(GetSize() > 1);
  MappingType pair = array[1];
  array[0] = array[1];
  Remove(1);

  // delegate to helper function
  recipient->CopyLastFrom(pair, buffer_pool_manager);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
CopyLastFrom(const MappingType &pair, BufferPoolManager *buffer_pool_manager) {
  assert(GetSize() + 1 < GetMaxSize());

  auto parent = reinterpret_cast<BPlusTreeInternalPage *>(
      buffer_pool_manager->FetchPage(GetParentPageId()));

  auto index = parent->ValueIndex(GetPageId());
  auto key = parent->KeyAt(index + 1);

  array[GetSize()] = {key, pair.second};
  IncreaseSize(1);
  parent->SetKeyAt(index + 1, pair.first);

  // unpin when we are done
  buffer_pool_manager->UnpinPage(parent->GetPageId(), true);
}

/*
 * Remove the last key & value pair from this page to head of "recipient"
 * page, then update relevant key & value pair in its parent page.
 */
template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
MoveLastToFrontOf(BPlusTreeInternalPage *recipient, int parent_index,
                  BufferPoolManager *buffer_pool_manager) {
  assert(GetSize() > 1);
  IncreaseSize(-1);
  MappingType pair = array[GetSize()];
  recipient->CopyFirstFrom(pair, parent_index, buffer_pool_manager);
}

template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
CopyFirstFrom(const MappingType &pair, int parent_index,
              BufferPoolManager *buffer_pool_manager) {
  assert(GetSize() + 1 < GetMaxSize());

  auto parent = reinterpret_cast<BPlusTreeInternalPage *>(
      buffer_pool_manager->FetchPage(GetParentPageId()));

  auto key = parent->KeyAt(parent_index);

  // set parent key to the last of current page
  parent->SetKeyAt(parent_index, pair.first);

  InsertNodeAfter(array[0].second, key, array[0].second);
  array[0].second = pair.second;

  buffer_pool_manager->UnpinPage(parent->GetPageId(), true);
}

/*****************************************************************************
 * DEBUG
 *****************************************************************************/
template <typename KeyType, typename ValueType, typename KeyComparator>
void BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
QueueUpChildren(std::queue<BPlusTreePage *> *queue,
                BufferPoolManager *buffer_pool_manager) {
  for (int i = 0; i < GetSize(); i++) {
    auto *page = buffer_pool_manager->FetchPage(array[i].second);
    if (page == nullptr)
      throw Exception(EXCEPTION_TYPE_INDEX,
                      "all page are pinned while printing");
    auto *node = reinterpret_cast<BPlusTreePage *>(page->GetData());
    queue->push(node);
  }
}

template <typename KeyType, typename ValueType, typename KeyComparator>
std::string BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>::
ToString(bool verbose) const {
  if (GetSize() == 0) {
    return "";
  }
  std::ostringstream os;
  if (verbose) {
    os << "[pageId: " << GetPageId() << " parentId: "
       << GetParentPageId() << "]<" << GetSize() << "> ";
  }

  int entry = verbose ? 0 : 1;
  int end = GetSize();
  bool first = true;
  while (entry < end) {
    if (first) {
      first = false;
    } else {
      os << " ";
    }
    os << std::dec << array[entry].first.ToString();
    if (verbose) {
      os << "(" << array[entry].second << ")";
    }
    ++entry;
  }
  return os.str();
}

// valuetype for internalNode should be page id_t
template class BPlusTreeInternalPage<GenericKey<4>, page_id_t, GenericComparator<4>>;
template class BPlusTreeInternalPage<GenericKey<8>, page_id_t, GenericComparator<8>>;
template class BPlusTreeInternalPage<GenericKey<16>, page_id_t, GenericComparator<16>>;
template class BPlusTreeInternalPage<GenericKey<32>, page_id_t, GenericComparator<32>>;
template class BPlusTreeInternalPage<GenericKey<64>, page_id_t, GenericComparator<64>>;

} // namespace cmudb
