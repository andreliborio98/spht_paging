#if!defined BPLUSTREE_HPP_227824
#define BPLUSTREE_HPP_227824

/*
* On older Linux systems, this is required for glibc to define
* std::posix_memalign #if !defined(_XOPEN_SOURCE) || (_XOPEN_SOURCE < 600)
* #define _XOPEN_SOURCE 600 #endif
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

//
// #include <boost/pool/object_pool.hpp>

#include "tm.h"

//DEBUG
#include <iostream>
using std::cout;
using std::endl;

#ifdef __linux__
#define HAVE_POSIX_MEMALIGN
#endif

#ifdef HAVE_POSIX_MEMALIGN
//Nothing to do
#else
//TODO:This is not aligned ! It doesn 't matter for this implementation, but it could
static inline int
posix_memalign(void **result, int, size_t bytes)
{
  *result = S_MALLOC(bytes);
  return *result == NULL;
}
#endif

template < typename KEY, typename VALUE, unsigned N, unsigned M, unsigned INNER_NODE_PADDING = 0,
unsigned LEAF_NODE_PADDING = 0, unsigned NODE_ALIGNMENT = 64 >
class BPlusTree
{
public:
  //N must be greater than two to make the split of
  // two inner nodes sensible.
  // BOOST_STATIC_ASSERT(N > 2);
  //Leaf nodes must be able to hold at least one element
  // BOOST_STATIC_ASSERT(M > 0);

  //Builds a new empty tree.
  BPlusTree() : depth(0), root(new_leaf_node_single()) {
    //DEBUG
    // cout << "sizeof(LeafNode)==" << sizeof(LeafNode) << endl;
    //cout << "sizeof(InnerNode)==" << sizeof(InnerNode) << endl;
  }

  ~BPlusTree() {
    //Empty.Memory deallocation is done automatically
    // when innerPool and leafPool are destroyed.
  }
  
  void insert_single(KEY key, VALUE value) {
    //GCC warns that this may be used uninitialized, even though that is untrue.
    InsertionResult result = {KEY(), 0, 0};
    bool was_split;
    unsigned long d = depth;
    if (d == 0) {
      //The root is a leaf node
      void *temp_node = root;
      was_split = leaf_insert_single(reinterpret_cast < LeafNode * >(temp_node), key, value, &result);
    } else {
      //The root is an inner node
      void *temp_node = root;
      was_split = inner_insert_single(reinterpret_cast < InnerNode * >(temp_node), d, key, value, &result);
    }
    if (was_split) {
      //The old root was splitted in two parts.
      // We have to create a new root pointing to them
      void *temp_node_2 = new_inner_node_single();
      InnerNode *rootProxy = reinterpret_cast < InnerNode * >(temp_node_2);

      rootProxy->num_keys = 1;
      rootProxy->keys[0] = result.key;
      rootProxy->children[0] = result.left;
      rootProxy->children[1] = result.right;
      depth = ++d;
      root = rootProxy;
    }
  }

  void insert(KEY key, VALUE value) {
    //GCC warns that this may be used uninitialized, even though that is untrue.
    InsertionResult result = {KEY(), 0, 0};
    bool was_split;
    unsigned long d = depth;
    if (d == 0) {
      //The root is a leaf node
      void *temp_node = root;
      was_split = leaf_insert(reinterpret_cast < LeafNode * >(temp_node), key, value, &result);
    } else {
      //The root is an inner node
      void *temp_node = root;
      was_split = inner_insert(reinterpret_cast < InnerNode * >(temp_node), d, key, value, &result);
    }
    if (was_split) {
      //The old root was splitted in two parts.
      // We have to create a new root pointing to them
      void *temp_node_2 = new_inner_node();
      InnerNode *rootProxy = reinterpret_cast < InnerNode * >(temp_node_2);

      //rootProxy->num_keys = 1;
      TM_SHARED_WRITE(rootProxy->num_keys, 1);
      //rootProxy->keys[0] = result.key;
      TM_SHARED_WRITE(rootProxy->keys[0], result.key);
      //rootProxy->children[0] = result.left;
      TM_SHARED_WRITE(rootProxy->children[0], result.left);
      //rootProxy->children[1] = result.right;
      TM_SHARED_WRITE(rootProxy->children[1], result.right);
      depth = ++d;
      root = rootProxy;
    }
  }

  
  //Looks for the given key.If it is not found, it returns false,
  //if it is found, it returns true and copies the associated value
  // unless the pointer is null.
  __attribute__((transaction_safe)) bool fast_find(TM_ARGDECL const KEY & key, VALUE * value = 0)const {
    const InnerNode * inner;
    register const void *node = FAST_PATH_SHARED_READ_P(root);
    register unsigned long d = FAST_PATH_SHARED_READ(depth);
    register unsigned index;
    while (d-- != 0) {
      inner = reinterpret_cast < const InnerNode *>(node);
      unsigned long num_keys_1 = FAST_PATH_SHARED_READ(inner->num_keys);
      index = fast_inner_position_for(TM_ARG key, inner->keys, num_keys_1);
      node = FAST_PATH_SHARED_READ_P(inner->children[index]);
    }
    const LeafNode *leaf = reinterpret_cast < const LeafNode * >(node);
    unsigned long num_keys = FAST_PATH_SHARED_READ(leaf->num_keys);

    index = fast_leaf_position_for(TM_ARG key, leaf->keys, num_keys);
    if (index < num_keys) {
      intptr_t temp_key = FAST_PATH_SHARED_READ(leaf->keys[index]);
      if (temp_key == key) {
        VALUE temp_value = (VALUE) FAST_PATH_SHARED_READ_P(leaf->values[index]);

        if (value != 0) {
          *value = temp_value;
        }
        if (temp_value)
        return true;
        else
        return false;
      }
    } else {
      return false;
    }
  }


  //Looks for the given key.If it is not found, it returns false,
  //if it is found, it returns true and sets
  // the associated value to NULL
  // Note:del currently leaks memory.Fix later.
  __attribute__((transaction_safe)) bool fast_del(TM_ARGDECL const KEY & key) {
    InnerNode *inner;
    register void *node = FAST_PATH_SHARED_READ_P(root);
    register unsigned long d = FAST_PATH_SHARED_READ(depth);
    register unsigned index;
    while (d-- != 0) {
      inner = reinterpret_cast < InnerNode * >(node);
      unsigned long num_keys_1 = FAST_PATH_SHARED_READ(inner->num_keys);
      index = fast_inner_position_for(TM_ARG key, inner->keys, num_keys_1);
      node = FAST_PATH_SHARED_READ_P(inner->children[index]);
    }
    LeafNode *leaf = reinterpret_cast < LeafNode * >(node);
    unsigned long num_keys = FAST_PATH_SHARED_READ(leaf->num_keys);

    index = fast_leaf_position_for(TM_ARG key, leaf->keys, num_keys);
    intptr_t temp_key = FAST_PATH_SHARED_READ(leaf->keys[index]);

    if (temp_key == key) {
      FAST_PATH_SHARED_WRITE_P(leaf->values[index], 0);
      return true;
    } else {
      return false;
    }
    // TODO: add freed nodes to an array
    // freedLeafNodes.push_back(leaf->children[index]);
  }


  //Finds the LAST item that is < key.That is, the next item in the tree is not < key, but this
  // item is.If we were to insert key into the tree, it would go after this item.This is weird,
  //but is easier than implementing iterators.In STL terms, this would be "lower_bound(key)--"
  // WARNING:This does * not * work when values are deleted.Thankfully, TPC - C does not use deletes.
  __attribute__((transaction_safe)) bool fast_findLastLessThan(TM_ARGDECL const KEY & key, VALUE * value = 0, KEY * out_key = 0)const {
    const void *node = FAST_PATH_SHARED_READ_P(root);
    unsigned long d = FAST_PATH_SHARED_READ(depth);
    while (d-- != 0) {
      const InnerNode *inner = reinterpret_cast < const InnerNode * >(node);
      unsigned long num_keys_1 = FAST_PATH_SHARED_READ(inner->num_keys);
      unsigned int pos = fast_inner_position_for(TM_ARG key, inner->keys, num_keys_1);
      //We need to rewind in the case where they are equal
      if  (pos > 0) {
        intptr_t temp_key = FAST_PATH_SHARED_READ(inner->keys[pos - 1]);
        if (key == temp_key)
        pos -= 1;
      }
      node = FAST_PATH_SHARED_READ_P(inner->children[pos]);
    }
    const LeafNode *leaf = reinterpret_cast < const LeafNode * >(node);
    unsigned long num_keys = FAST_PATH_SHARED_READ(leaf->num_keys);
    unsigned int pos = fast_leaf_position_for(TM_ARG key, leaf->keys, num_keys);

    if (pos <= num_keys) {
      pos -= 1;
      if (pos < num_keys) {
        intptr_t temp_key_2 = FAST_PATH_SHARED_READ(leaf->keys[pos]);

        if (key == temp_key_2)
        pos -= 1;
      }
      if (pos < num_keys) {
        VALUE temp_value = (VALUE) FAST_PATH_SHARED_READ_P(leaf->values[pos]);

        if (temp_value) {
          if (value != NULL) {
            *value = temp_value;
          }
          if (out_key != NULL) {
            intptr_t temp_key_3 = FAST_PATH_SHARED_READ(leaf->keys[pos]);
            *out_key = temp_key_3;
          }
          return true;
        } else {
          //This is a deleted key ! Try again with the new key value
          // HACK:This is because this implementation doesn 't do deletes correctly. However,
          // this makes it work.We need this for TPC-C undo.The solution is to use a
          // more complete b - tree implementation.
          intptr_t temp_key_4 = FAST_PATH_SHARED_READ(leaf->keys[pos]);
          return fast_findLastLessThan(TM_ARG temp_key_4, value, out_key);
        }
      }
    }
    return false;
  }

  //Returns the size of an inner node
  // It is useful when optimizing performance with cache alignment.
  unsigned sizeof_inner_node() const {
    return sizeof(InnerNode);
  }

  //Returns the size of a leaf node.
  //    It is useful when optimizing performance with cache alignment.
  unsigned sizeof_leaf_node() const {
    return sizeof(LeafNode);
  }




private:

  // this should be versioned?
  // Since we have a very basic allocator that always return 0'ed memory (and
  // no deallocation happens), it might be safe for now ...
  __attribute__((transaction_safe)) static void *tm_memset(void *dst, int c, size_t n){
    if (n) {
      char *d = (char *)dst;

      do {
        *d++ = c;
      } while (--n);
    }
    return dst;
  }

  //Used when debugging
  enum NodeType {
    NODE_INNER = 0xDEADBEEF, NODE_LEAF = 0xC0FFEE};

    //Leaf nodes store pairs of keys and values.
    struct LeafNode {
      #ifndef NDEBUG
      //Leaf nodes need to memset the keys:when inserting the first key, we
      // check to see if we need to overwrite or insert
      LeafNode()
      :type(NODE_LEAF), num_keys(0) {
        tm_memset(keys, 0, sizeof(keys));
      }
      NodeType type;
      #else
      LeafNode():num_keys(0) {
        memset(keys, 0, sizeof(keys));
      }
      #endif
      unsigned long num_keys;
      KEY keys[M];
      VALUE values[M];
      //unsigned char _pad[LEAF_NODE_PADDING];
    };

    // Ideally we would like to add the writes to the log if we are in a
    // transactions - the problem is that this code is also called outside
    // a transaction ...
    __attribute__((transaction_safe)) void initLeafNode(LeafNode * node) {
      //node->type = NODE_LEAF;
      TM_SHARED_WRITE(node->type, NODE_LEAF);
      //node->num_keys = 0;
      TM_SHARED_WRITE(node->num_keys, 0);
      tm_memset(node->keys, 0, sizeof(node->keys));
    }
    
    void initLeafNode_single(LeafNode * node) {
      node->type = NODE_LEAF;
      node->num_keys = 0;
      tm_memset(node->keys, 0, sizeof(node->keys));
    }

    //Inner nodes store pointers to other nodes interleaved with keys.
    struct InnerNode {
      #ifndef NDEBUG
      InnerNode():type(NODE_INNER), num_keys(0) {
      }
      NodeType type;
      #else
      InnerNode():num_keys(0) {
      }
      #endif
      unsigned long num_keys;
      KEY keys[N];
      void *children[N + 1];
      //unsigned char _pad[INNER_NODE_PADDING];
    };

    // Ideally we would like to add the writes to the log if we are in a
    // transactions - the problem is that this code is also called outside
    // a transaction ...
    __attribute__((transaction_safe)) void initInnerNode(InnerNode * node) {
      //node->type = NODE_INNER;
      TM_SHARED_WRITE(node->type, NODE_INNER);
      //node->num_keys = 0;
      TM_SHARED_WRITE(node->num_keys, 0);
    }
    
    void initInnerNode_single(InnerNode * node) {
      node->type = NODE_INNER;
      node->num_keys = 0;
    }

    //Custom allocator that returns aligned blocks of memory
    template < unsigned ALIGNMENT >
    struct AlignedMemoryAllocator {
      typedef std::size_t size_type;
      typedef std::ptrdiff_t difference_type;

      static char *malloc(const size_type bytes){
        return S_MALLOC(bytes);
        // void *result;
        // if   (posix_memalign(&result, ALIGNMENT, bytes) != 0) {
        //   result = 0;
        // }
        // //Alternative:      result = std: :     malloc(bytes);
        // return reinterpret_cast < char *>(result);
      }
      static void free(char *const block){
        S_FREE(block);
      }
    };

    //Returns a pointer to a fresh leaf node.
    __attribute__((transaction_safe)) inline LeafNode *new_leaf_node() {
      LeafNode *result = (LeafNode *) P_MALLOC(sizeof(LeafNode));
      initLeafNode(result);
      //result = leafPool.construct();
      // cout << "New LeafNode at " << result << endl;
      return result;
    }
    
    inline LeafNode *new_leaf_node_single() {
      LeafNode *result = (LeafNode *) P_MALLOC(sizeof(LeafNode));
      initLeafNode_single(result);
      //result = leafPool.construct();
      // cout << "New LeafNode at " << result << endl;
      return result;
    }

    //Returns a pointer to a fresh inner node.
    __attribute__((transaction_safe)) inline InnerNode *new_inner_node() {
      InnerNode *result = (InnerNode *) P_MALLOC(sizeof(InnerNode));
      initInnerNode(result);
      //result = innerPool.construct();
      // cout << "New InnerNode at " << result << endl;
      return result;
    }
    
    inline InnerNode *new_inner_node_single() {
      InnerNode *result = (InnerNode *) P_MALLOC(sizeof(InnerNode));
      initInnerNode_single(result);
      //result = innerPool.construct();
      // cout << "New InnerNode at " << result << endl;
      return result;
    }

    //Data type returned by the private insertion methods.
    struct InsertionResult {
      KEY key;
      void *left;
      void *right;
    };

    //Returns the position where 'key' should be inserted in a leaf node
    // that has the given keys.
    static unsigned leaf_position_for(const KEY & key, const KEY * keys, unsigned long num_keys) {
      //Simple linear search.Faster for small values of N or M
      unsigned k = 0;
      intptr_t temp_key = keys[k];
      while ((k < num_keys) && (temp_key < key)) {
        ++k;
        temp_key = keys[k];
      }
      return k;
    }

    static unsigned fast_leaf_position_for(TM_ARGDECL const KEY & key, const KEY * keys, unsigned long num_keys){
      //Simple linear search.Faster for small values of N or M
      unsigned k = 0;
      intptr_t temp_key = FAST_PATH_SHARED_READ(keys[k]);
      while ((k < num_keys) && (temp_key < key)) {
        ++k;
        temp_key = FAST_PATH_SHARED_READ(keys[k]);
      }
      return k;
    }

    static inline unsigned inner_position_for(const KEY & key, const KEY * keys, unsigned long num_keys) {
      unsigned k = 0;
      intptr_t temp_key = keys[k];
      while ((k < num_keys) && ((temp_key < key) || (temp_key == key))) {
        ++k;
        temp_key = keys[k];
      }
      return k;
    }

    
    //Returns the position where 'key' should be inserted in an inner node
    // that has the given keys.
    static inline unsigned fast_inner_position_for(TM_ARGDECL const KEY & key, const KEY * keys, unsigned long num_keys){
      //Simple linear search.Faster for small values of N or M
      unsigned k = 0;
      intptr_t temp_key = FAST_PATH_SHARED_READ(keys[k]);
      while    ((k < num_keys) && ((temp_key < key) || (temp_key == key))) {
        ++k;
        temp_key = FAST_PATH_SHARED_READ(keys[k]);
      }
      return k;

      //Binary search is faster when N or M is > 100,
      //but the cost of the division renders it useless
      // for smaller values of N or M.
    }

    bool leaf_insert_single(LeafNode * node, KEY & key, VALUE & value, InsertionResult * result) {
      bool was_split = false;
      //Simple linear search
      unsigned long num_keys = node->num_keys;
      unsigned i = leaf_position_for(key, node->keys, num_keys);
      if (num_keys >= M) {
        //The node was full.We must split it
        unsigned treshold = (M + 1) / 2;
        LeafNode *new_sibling = new_leaf_node_single();
        new_sibling->num_keys = num_keys - treshold;
        for (unsigned j = 0; j < new_sibling->num_keys; ++j) {
          intptr_t temp_key = node->keys[treshold + j];
          new_sibling->keys[j] = temp_key;
          VALUE temp_value = (VALUE) node->values[treshold + j];
          new_sibling->values[j] = temp_value;
        }
        node->num_keys = treshold;

        if (i < treshold) {
          //Inserted element goes to left sibling
          leaf_insert_nonfull_single(node, key, value, i);
        } else {
          //Inserted element goes to right sibling
          leaf_insert_nonfull_single(new_sibling, key, value, i - treshold);
        }
        //Notify the parent about the split
        was_split = true;
        // result is not persistent
        result->key = new_sibling->keys[0];
        result->left = node;
        result->right = new_sibling;
      } else {
        //The node was not full
        leaf_insert_nonfull_single(node, key, value, i);
      }
      return was_split;
    }


    bool leaf_insert(LeafNode * node, KEY & key, VALUE & value, InsertionResult * result) {
      bool was_split = false;
      //Simple linear search
      unsigned long num_keys = node->num_keys;
      unsigned i = leaf_position_for(key, node->keys, num_keys);
      if (num_keys >= M) {
        //The node was full.We must split it
        unsigned treshold = (M + 1) / 2;
        LeafNode *new_sibling = new_leaf_node();
        //new_sibling->num_keys = num_keys - treshold;
        TM_SHARED_WRITE(new_sibling->num_keys, num_keys - treshold);
        for (unsigned j = 0; j < new_sibling->num_keys; ++j) {
          intptr_t temp_key = node->keys[treshold + j];
          //new_sibling->keys[j] = temp_key;
          TM_SHARED_WRITE(new_sibling->keys[j], temp_key);
          VALUE temp_value = (VALUE) node->values[treshold + j];
          //new_sibling->values[j] = temp_value;
          TM_SHARED_WRITE(new_sibling->values[j], temp_value);
        }
        //node->num_keys = treshold;
        TM_SHARED_WRITE(node->num_keys, treshold);

        if (i < treshold) {
          //Inserted element goes to left sibling
          leaf_insert_nonfull(node, key, value, i);
        } else {
          //Inserted element goes to right sibling
          leaf_insert_nonfull(new_sibling, key, value, i - treshold);
        }
        //Notify the parent about the split
        was_split = true;
        // result is not persistent
        result->key = new_sibling->keys[0];
        result->left = node;
        result->right = new_sibling;
      } else {
        //The node was not full
        leaf_insert_nonfull(node, key, value, i);
      }
      return was_split;
    }

    static void leaf_insert_nonfull_single(LeafNode * node, KEY & key, VALUE & value, unsigned index) {
      intptr_t temp_key = node->keys[index];
      if (temp_key == key) {
        //We are inserting a duplicate value.
        // Simply overwrite the old one
        VALUE temp_value = (VALUE) value;
        node->values[index] = temp_value;
      } else {
        unsigned long num_keys = node->num_keys;

        //The key we are inserting is unique
        for (unsigned i = num_keys; i > index; --i) {
          intptr_t temp_key_2 = node->keys[i - 1];

          node->keys[i] = temp_key_2;
          VALUE temp_value_2 = (VALUE) node->values[i - 1];

          node->values[i] = temp_value_2;
        }
        node->num_keys = ++num_keys;
        node->keys[index] = key;
        VALUE temp_value_3 = (VALUE) value;

        node->values[index] = temp_value_3;
      }
    }


    static void leaf_insert_nonfull(LeafNode * node, KEY & key, VALUE & value, unsigned index) {
      intptr_t temp_key = node->keys[index];
      if (temp_key == key) {
        //We are inserting a duplicate value.
        // Simply overwrite the old one
        VALUE temp_value = (VALUE) value;
        //node->values[index] = temp_value;
        TM_SHARED_WRITE_P(node->values[index], temp_value);
      } else {
        unsigned long num_keys = node->num_keys;

        //The key we are inserting is unique
        for (unsigned i = num_keys; i > index; --i) {
          intptr_t temp_key_2 = node->keys[i - 1];

          //node->keys[i] = temp_key_2;
          TM_SHARED_WRITE(node->keys[i], temp_key_2);
          VALUE temp_value_2 = (VALUE) node->values[i - 1];

          //node->values[i] = temp_value_2;
          TM_SHARED_WRITE(node->values[i], temp_value_2);
        }
        //node->num_keys = ++num_keys;
        num_keys++;
        TM_SHARED_WRITE(node->num_keys, num_keys);
        //node->keys[index] = key;
        TM_SHARED_WRITE(node->keys[index], key);
        VALUE temp_value_3 = (VALUE) value;

        //node->values[index] = temp_value_3;
        TM_SHARED_WRITE_P(node->values[index], temp_value_3);
      }
    }

    bool inner_insert_single(InnerNode * node, unsigned long current_depth, KEY & key, VALUE & value, InsertionResult * result){
      //Early split if node is full.
      // This is not the canonical algorithm for B + trees,
      //but it is simpler and does not break the definition.
      bool was_split = false;
      unsigned long num_keys = node->num_keys;
      if (num_keys >= N) {
        //Split
        unsigned treshold = (N + 1) / 2;
        InnerNode *new_sibling = new_inner_node_single();
        new_sibling->num_keys = num_keys - treshold;
        for (unsigned i = 0; i < new_sibling->num_keys; ++i) {
          intptr_t temp_key = node->keys[treshold + i];
          new_sibling->keys[i] = temp_key;
          new_sibling->children[i] = node->children[treshold + i];
        }
        new_sibling->children[new_sibling->num_keys] = node->children[num_keys];

        node->num_keys = treshold - 1;
        //Set up the return variable
        was_split = true;
        intptr_t temp_key_2 = node->keys[treshold - 1];

        result->key = temp_key_2;
        result->left = node;
        result->right = new_sibling;
        //Now insert in the appropriate sibling
        if (key < result->key) {
          inner_insert_nonfull_single(node, current_depth, key, value);
        } else {
          inner_insert_nonfull_single(new_sibling, current_depth, key, value);
        }
      } else {
        //No split
        inner_insert_nonfull_single(node, current_depth, key, value);
      }
      return was_split;
    }


    bool inner_insert(InnerNode * node, unsigned long current_depth, KEY & key, VALUE & value, InsertionResult * result){
      //Early split if node is full.
      // This is not the canonical algorithm for B + trees,
      //but it is simpler and does not break the definition.
      bool was_split = false;
      unsigned long num_keys = node->num_keys;
      if (num_keys >= N) {
        //Split
        unsigned treshold = (N + 1) / 2;
        InnerNode *new_sibling = new_inner_node();
        //new_sibling->num_keys = num_keys - treshold;
        TM_SHARED_WRITE(new_sibling->num_keys, num_keys - treshold);
        for (unsigned i = 0; i < new_sibling->num_keys; ++i) {
          intptr_t temp_key = node->keys[treshold + i];
          //new_sibling->keys[i] = temp_key;
          TM_SHARED_WRITE(new_sibling->keys[i], temp_key);
          //new_sibling->children[i] = node->children[treshold + i];
          TM_SHARED_WRITE(new_sibling->children[i], 
              node->children[treshold+i]);
        }
        //new_sibling->children[new_sibling->num_keys] = node->children[num_keys];
        TM_SHARED_WRITE(new_sibling->children[new_sibling->num_keys], 
              node->children[num_keys]);

        //node->num_keys = treshold - 1;
        TM_SHARED_WRITE(node->num_keys, treshold - 1);
        //Set up the return variable
        was_split = true;
        intptr_t temp_key_2 = node->keys[treshold - 1];

        result->key = temp_key_2;
        result->left = node;
        result->right = new_sibling;
        //Now insert in the appropriate sibling
        if (key < result->key) {
          inner_insert_nonfull(node, current_depth, key, value);
        } else {
          inner_insert_nonfull(new_sibling, current_depth, key, value);
        }
      } else {
        //No split
        inner_insert_nonfull(node, current_depth, key, value);
      }
      return was_split;
    }


    void inner_insert_nonfull_single(InnerNode * node, unsigned long current_depth, KEY & key, VALUE & value) {
      //Simple linear search
      unsigned long num_keys = node->num_keys;
      unsigned index = inner_position_for(key, node->keys, num_keys);
      //GCC warns that this may be used uninitialized, even though that is untrue.
      InsertionResult result = {
        KEY(), 0, 0
      };
      bool was_split;

      if (current_depth - 1 == 0) {
        //The children are leaf nodes
        void *temp_node = node->children[index];
        
        was_split = leaf_insert_single(reinterpret_cast < LeafNode * >(temp_node), key, value, &result);
      } else {
        //The children are inner nodes
        void *temp_node = node->children[index];
        
        InnerNode *child = reinterpret_cast < InnerNode * >(temp_node);

        was_split = inner_insert_single(TM_ARG child, current_depth - 1, key, value, &result);
      }
      if (was_split) {
        if (index == num_keys) {
          //Insertion at the rightmost key
          node->keys[index] = result.key;
          node->children[index] = result.left;
          node->children[index + 1] = result.right;
          node->num_keys = ++num_keys;
        } else {
          //Insertion not at the rightmost key
          void *temp_node_2 = node->children[num_keys];

          node->children[num_keys + 1] = temp_node_2;
          for (unsigned i = num_keys; i != index; --i) {
            void *temp_node_3 = node->children[i - 1];

            node->children[i] = temp_node_3;
            intptr_t temp_key_2 = node->keys[i - 1];

            node->keys[i] = temp_key_2;
          }
          node->children[index] = result.left;
          node->children[index + 1] = result.right;
          node->keys[index] = result.key;
          node->num_keys = ++num_keys;
        }
      } // else the current node is not affected
    }

    void inner_insert_nonfull(InnerNode * node, unsigned long current_depth, KEY & key, VALUE & value) {
      //Simple linear search
      unsigned long num_keys = node->num_keys;
      unsigned index = inner_position_for(key, node->keys, num_keys);
      //GCC warns that this may be used uninitialized, even though that is untrue.
      InsertionResult result = {
        KEY(), 0, 0
      };
      bool was_split;

      if (current_depth - 1 == 0) {
        //The children are leaf nodes
        void *temp_node = node->children[index];
       
        was_split = leaf_insert(reinterpret_cast < LeafNode * >(temp_node), key, value, &result);
      } else {
        //The children are inner nodes
        void *temp_node = node->children[index];
       
        InnerNode *child = reinterpret_cast < InnerNode * >(temp_node);

        was_split = inner_insert(TM_ARG child, current_depth - 1, key, value, &result);
      }
      if (was_split) {
        if (index == num_keys) {
          //Insertion at the rightmost key
          //node->keys[index] = result.key;
          TM_SHARED_WRITE(node->keys[index], result.key);
          //node->children[index] = result.left;
          TM_SHARED_WRITE_P(node->children[index], result.left);
          //node->children[index + 1] = result.right;
          TM_SHARED_WRITE_P(node->children[index + 1], result.right);
          //node->num_keys = ++num_keys;
          num_keys++;
          TM_SHARED_WRITE(node->num_keys, num_keys);
        } else {
          //Insertion not at the rightmost key
          void *temp_node_2 = node->children[num_keys];

          //node->children[num_keys + 1] = temp_node_2;
          TM_SHARED_WRITE_P(node->children[num_keys + 1], temp_node_2);
          for (unsigned i = num_keys; i != index; --i) {
            void *temp_node_3 = node->children[i - 1];

            //node->children[i] = temp_node_3;
            TM_SHARED_WRITE_P(node->children[i], temp_node_3);
            intptr_t temp_key_2 = node->keys[i - 1];

            //node->keys[i] = temp_key_2;
            TM_SHARED_WRITE(node->keys[i], temp_key_2);
          }
          //node->children[index] = result.left;
          TM_SHARED_WRITE_P(node->children[index], result.left);
          //node->children[index + 1] = result.right;
          TM_SHARED_WRITE_P(node->children[index + 1], result.right);
          //node->keys[index] = result.key;
          TM_SHARED_WRITE(node->keys[index], result.key);
          //node->num_keys = ++num_keys;
          num_keys++;
          TM_SHARED_WRITE(node->num_keys, num_keys);
        }
      } // else the current node is not affected
    }

    typedef AlignedMemoryAllocator < NODE_ALIGNMENT > AlignedAllocator;

    //Node memory allocators.IMPORTANT NOTE:they must be declared
    // before the root to make sure that they are properly initialised
    // before being used to allocate any node.
    // boost: :object_pool < InnerNode, AlignedAllocator > innerPool;
    //boost: :object_pool < LeafNode, AlignedAllocator > leafPool;
    //Depth of the tree.A tree of depth 0 only has a leaf node.
    unsigned long depth;

    //Pointer to the root node.It may be a leaf or an inner node, but
    // it is never null.
    void *root;

    // thread_local std::vector< InnerNode* > freedInnerNodes; 
    // thread_local std::vector< LeafNode* > freedLeafNodes; 
  };

  #endif        /* // !defined BPLUSTREE_HPP_227824 */
