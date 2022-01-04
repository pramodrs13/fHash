#ifndef _PRAMOD_HASH_MAP_
#define _PRAMOD_HASH_MAP_

#include "stdio.h"
#include "stdlib.h"
#include <stdint.h>
#include <vector>

/* The golden ration: an arbitrary value */
#define JHASH_GOLDEN_RATIO  0x9e3779b9
#define __jhash_mix(a, b, c) \
{ \
	a -= b; a -= c; a ^= (c>>13); \
	b -= c; b -= a; b ^= (a<<8); \
	c -= a; c -= b; c ^= (b>>13); \
	a -= b; a -= c; a ^= (c>>12);  \
	b -= c; b -= a; b ^= (a<<16); \
	c -= a; c -= b; c ^= (b>>5); \
	a -= b; a -= c; a ^= (c>>3);  \
	b -= c; b -= a; b ^= (a<<10); \
	c -= a; c -= b; c ^= (b>>15); \
}
//	JenkinsHash is standard opensource code for calculatin hash function
static inline uint32_t JenkinsHash(const void* type, uint32_t length, uint32_t initval)
{
	uint32_t a, b, c, len;
	const uint32_t * k = static_cast<const uint32_t *>(type);

	a = b = JHASH_GOLDEN_RATIO;
	c = initval;
	len = length;

	while (len >= 3)
	{
		a += k[0];
		b += k[1];
		c += k[2];
		__jhash_mix(a, b, c);
		k += 3;
		len -= 3;
	}

	c += length * 4;

	switch (len)
	{
		case 2:
			b += k[1];
		case 1:
			a += k[0];
	};

	__jhash_mix(a, b, c);

	return c;
}



#define _HASH_TABLE_SIZE 10000
#define GETKEYHASH(key,len,hash) hash=JenkinsHash(key,len,0);hash %= _HASH_TABLE_SIZE;

class HashNode;
template <class Key, class Value>
class PramodHashMap
{

	public:
		//A particular enty into hash map
		class HashNode
		{
			private:
				Key key;
				Value value;
			public:
				HashNode *next;
				uint16_t hashTableIndex;

			public:

				HashNode(Key key, Value value, uint16_t hashTableIndex)
				{
					this->key = key;
					this->value = value;
					this->hashTableIndex = hashTableIndex;
					this->next = NULL;
				}

				~HashNode()
				{
				}

				Key& getKey()
				{
					return key;
				}

				Value getValue()
				{
					return value;
				}

				void setValue(Value& value)
				{
					this->value = value;
				}

				HashNode *getNext()
				{
					return next;
				}

				void setNext(HashNode *next)
				{
					this->next = next;
				}


		};

	private:
		HashNode* hashTable[_HASH_TABLE_SIZE];
		uint32_t hashCollisionCntrTable[_HASH_TABLE_SIZE];
		uint32_t _size;


	public:

		PramodHashMap()
		{
			for (int i = 0; i < _HASH_TABLE_SIZE; i++)
			{
				hashTable[i] = NULL;
				hashCollisionCntrTable[i] = 0;
			}
			_size = 0;
		}

		//this refrence version will be used only with iterator
		inline Value& get(const Key& key) const
		{
			uint32_t hash = 0;
			GETKEYHASH((void*)&key,sizeof(key),hash);
			if (hashTable[hash] != NULL)
			{
				HashNode *node = hashTable[hash];
				while (node != NULL && node->getKey() != key)
				{
					node = node->getNext();
				}
				if (node != NULL)
				{
					return node->getValue();
				}
			}
		}

		//pointer version to get value
		inline Value  get(const Key& key)
		{
			uint32_t hash = 0;
			GETKEYHASH((void*)&key,sizeof(key),hash);
			if (hashTable[hash] != NULL)
			{
				HashNode *node = hashTable[hash];
				while ((node != NULL) && (node->getKey() != key))
				{
					node = node->getNext();
				}
				if (node != NULL)
				{
					return node->getValue();
				}
			}
			// printf("Hash Table contains no value for key, %d\n", key);
			return 0;
		}


		//overloaded operator []  to get value and it will be used with iterator
		inline Value& operator[](const Key &key) const
		{
			return get(key);
		}

		// Returns an array of all of the keys in the table.
		// we can iterate over the keys to get the values.
		void getKeys(std::vector<Key>& keyVector) const
		{
			keyVector.clear();
			for( uint32_t i = 0; i < _HASH_TABLE_SIZE; i++ )
			{
				HashNode* node = hashTable[i];
				while (node != NULL)
				{
					keyVector.push_back( node->key );
					node = node->next;
				}
			}
		}

		inline HashNode* getFirstNode()
		{
			for( uint32_t i = 0; i < _HASH_TABLE_SIZE; i++ )
			{
				HashNode* node = hashTable[i];
				if (node != NULL)
				{
					return node;
				}
			}
			return NULL;
		}

		//getNextNode will always give next HashNode. It will immediately return next of hNode in case if it is not NULL and if NULL then
		//it will go to next bucket and wil literate from there
		inline void getNextNode(HashNode*& hNode) const
		{
			if(hNode!=NULL)
			{
				if(hNode->next!=NULL)
				{
					hNode = hNode->next;
					return ;
				}
				else
				{
					uint16_t index = hNode->hashTableIndex;
					//index is increased by 1 because if hNode->next==NULL it means this will be last node at that index so we have to iterate from next index onward
					index += 1;
					for( index; index < _HASH_TABLE_SIZE; index++ )
					{
						HashNode* node = hashTable[index];
						if(node!=NULL)
						{
							hNode = node;
							return;
						}
					}
					hNode = NULL;
				}
			}
		}

		//Insert function will always add key/value pair and it wil lnever check whether input key is already present or not
		inline bool insert(const Key& key, Value& value)
		{
			uint32_t hash = 0;
			GETKEYHASH((void*)&key,sizeof(key),hash);
			HashNode *node = hashTable[hash];
			if (node == NULL)
			{
				hashTable[hash] = new HashNode(key, value, hash);
				_size ++;
			}
			else
			{
				node = hashTable[hash];
				//add node at the head of the list
				HashNode *newNode = new HashNode(key, value, hash);
				newNode->setNext(node);
				hashTable[hash] = newNode;
				hashCollisionCntrTable[hash] ++;
				//increment global collision counter
				_size ++;
			}
			return true;
		}

		//findAndInsert function will check whether key already exists or not and then it will update  or return false
		//in this function node in list will be added at tail
		inline bool findAndInsert(const Key& key, Value& value)
		{
			bool isInserted=true;
			uint32_t hash = 0;
			GETKEYHASH((void*)&key,sizeof(key),hash);
			if (hashTable[hash] == NULL)
			{
				hashTable[hash] = new HashNode(key, value, hash);
				_size ++;
			}
			else
			{
				HashNode *node = hashTable[hash];
				HashNode *prvNode = NULL;
				do
				{
					//this check is put to see whether insert is called for duplicate key. in case of duplicate key it will return false
					prvNode = node;
					if (node->getKey() == key)
					{
						//value will not be updated and false will be returned to user
						isInserted = false;
						break;
					}
					node = node->getNext();
				}while(node != NULL);

				if(isInserted)
				{
					prvNode->setNext(new HashNode(key, value, hash));
					//increment collision counter per bucket/list slot
					hashCollisionCntrTable[hash] ++;
					//increment global collision counter
					_size ++;
				}
			}
			return isInserted;
		}

		inline bool remove(const Key& key)
		{
			uint32_t hash = 0;
			GETKEYHASH((void*)&key,sizeof(key),hash);
			if (hashTable[hash] != NULL)
			{
				HashNode *prevnode = NULL;
				HashNode *node = hashTable[hash];
				while ((node->getNext() != NULL) && (node->getKey() != key))
				{
					prevnode = node;
					node = node->getNext();
				}
				if (node->getKey() == key)
				{
					if (prevnode == NULL)
					{
						HashNode *nextnode = node->getNext();
						hashTable[hash] = nextnode;
					}
					else
					{
						HashNode *next = node->getNext();
						prevnode->setNext(next);
					}
					//decrement collision counter per slot
					hashCollisionCntrTable[hash] --;
					//decrement collision counter globally
					_size --;
					delete node;
					return true;
				}
			}
			return false;
		}

		inline void clear()
		{
			freeHashTableMemory();
		}

		~PramodHashMap()
		{
			freeHashTableMemory();
		}

		inline void freeHashTableMemory()
		{
			for (int i = 0; i < _HASH_TABLE_SIZE; i++)
			{
				if (hashTable[i] != NULL)
				{
					HashNode *prevnode = NULL;
					HashNode *node = hashTable[i];
					while (node != NULL)
					{
						prevnode = node;
						node = node->getNext();
						delete prevnode;
					}
				}
				hashTable[i] = NULL;
				hashCollisionCntrTable[i] = 0;
			}
			_size = 0;
		}
		//numer of entries present into hashMap
		inline uint32_t size() const
		{
			return _size;
		}
		inline void incrementHashMapSize()
		{
			++_size;
		}
		inline void decrementHashMapSize()
		{
			--_size;
		}

		inline uint32_t debugGetTotalCollision()
		{
			uint32_t total_collision = 0;
			for (int i = 0; i < _HASH_TABLE_SIZE; i++)
			{
				total_collision += hashCollisionCntrTable[i];
			}
			return total_collision;
		}

		inline void debugPrintCollisionStats()
		{
			for (uint32_t i =0; i< _HASH_TABLE_SIZE; i++)
			{
				printf ("number of collision at index %u are %u\n",i,hashCollisionCntrTable[i]);
			}
		}

		inline uint32_t debugGetDeepestLinkSize() const
		{
			uint32_t deepest = 0;
			for (uint32_t loop = 0; loop < _HASH_TABLE_SIZE; loop++)
			{
				uint32_t     count = 0;
				HashNode *node = hashTable[loop];
				while (node != NULL)
				{
					node = node->next;
					count ++;
				}
				if (count > deepest)
				{
					deepest = count;
				}
			}
			return deepest;
		}

};
#endif /* _PRAMOD_HASH_MAP_ */
