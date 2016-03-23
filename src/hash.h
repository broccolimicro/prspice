/*
 * hashmap.h
 *
 *  Created on: Jun 20, 2015
 *      Author: nbingham
 */

#include <vector>
#include <list>
#include <string>
#include <map>
#include <sys/types.h>
#include <stdint.h>

using namespace std;

#ifndef common_hashmap_h
#define common_hashmap_h

struct hasher
{
	hasher();
	
	template <class type>
	hasher(type v)
	{
		put(*this, v);
	}

	template <class type>
	hasher(const type *v, int n)
	{
		put(*this, v, n);
	}

	~hasher();

	vector<char> data;

	uint32_t get();
};

void put(hasher &h, char v);
void put(hasher &h, unsigned char v);
void put(hasher &h, short v);
void put(hasher &h, unsigned short v);
void put(hasher &h, int v);
void put(hasher &h, unsigned int v);
void put(hasher &h, long v);
void put(hasher &h, unsigned long v);
void put(hasher &h, long long v);
void put(hasher &h, unsigned long long v);
void put(hasher &h, bool v);
void put(hasher &h, float v);
void put(hasher &h, double v);
void put(hasher &h, string v);

template <class type>
void put(hasher &h, type *v, int n = 1)
{
	for (int i = 0; i < n; i++)
		put(h, *(v+i));
}

template <class type0, class type1>
void put(hasher &h, const pair<type0, type1> &v)
{
	put(h, v.first);
	put(h, v.second);
}

template <class type>
void put(hasher &h, const vector<type> &v)
{
	for (typename vector<type>::const_iterator i = v.begin(); i != v.end(); i++)
		put(h, *i);
}

template <class type>
void put(hasher &h, const list<type> &v)
{
	for (typename vector<type>::const_iterator i = v.begin(); i != v.end(); i++)
		put(h, *i);
}

template <class key_type, class value_type>
void put(hasher &h, const map<key_type, value_type> &v)
{
	for (typename map<key_type, value_type>::const_iterator i = v->begin(); i != v->end(); i++)
		put(h, *i);
}

template <class key_type, class value_type, int num_buckets>
struct hashmap
{
	hashmap() : capacity(num_buckets)
	{
		count = 0;
	}

	~hashmap() {}

	const int capacity;
	map<key_type, value_type> buckets[num_buckets];
	int count;

	int size()
	{
		return count;
	}

	bool insert(const key_type &key, const value_type &value, typename map<key_type, value_type>::iterator* loc = NULL)
	{
		int bucket = hasher(key).get()%num_buckets;
		pair<typename map<key_type, value_type>::iterator, bool> result = buckets[bucket].insert(pair<key_type, value_type>(key, value));
		if (loc != NULL)
			*loc = result.first;
		count++;
		return result.second;
	}

	bool find(const key_type &key, typename map<key_type, value_type>::iterator* loc = NULL)
	{
		int bucket = hasher(key).get()%num_buckets;
		typename map<key_type, value_type>::iterator result = buckets[bucket].find(key);
		if (loc != NULL)
			*loc = result;
		return (result != buckets[bucket].end());
	}

	void erase(key_type key)
	{
		int bucket = hasher(key).get()%num_buckets;
		typename map<key_type, value_type>::iterator result = buckets[bucket].find(key);
		if (result != buckets[bucket].end())
			buckets[bucket].erase(result);
	}

	int max_bucket_size()
	{
		int max_size = 0;
		for (int i = 0; i < num_buckets; i++)
			if ((int)buckets[i].size() > max_size)
				max_size = buckets[i].size();
		return max_size;
	}

	hashmap<key_type, value_type, num_buckets> &operator=(const hashmap<key_type, value_type, num_buckets> &copy)
	{
		for (int i = 0; i < num_buckets; i++)
			buckets[i] = copy.buckets[i];
		count = copy.count;
		return *this;
	}
};

template <class value_type, int num_buckets>
struct hashtable
{
	hashtable() : capacity(num_buckets)
	{
		count = 0;
	}

	~hashtable() {}

	const int capacity;
	vector<value_type> buckets[num_buckets];
	int count;

	int size()
	{
		return count;
	}

	bool insert(const value_type &value, typename vector<value_type>::iterator *loc = NULL)
	{
		int bucket = hasher(value).get()%num_buckets;
		typename vector<value_type>::iterator result = lower_bound(buckets[bucket].begin(), buckets[bucket].end(), value);
		if (result == buckets[bucket].end() || *result != value)
		{
			result = buckets[bucket].insert(result, value);
			count++;
			if (loc != NULL)
				*loc = result;

			return true;
		}

		if (loc != NULL)
			*loc = result;

		return false;
	}

	bool contains(const value_type &value, typename vector<value_type>::iterator *loc = NULL)
	{
		int bucket = hasher(value).get()%num_buckets;
		typename vector<value_type>::iterator result = lower_bound(buckets[bucket].begin(), buckets[bucket].end(), value);
		if (loc != NULL)
			*loc = result;

		return (result != buckets[bucket].end() && *result == value);
	}

	void erase(const value_type &value)
	{
		int bucket = hasher(value).get()%num_buckets;
		typename vector<value_type>::iterator result = lower_bound(buckets[bucket].begin(), buckets[bucket].end(), value);
		buckets[bucket].erase(result);
	}

	int max_bucket_size()
	{
		int max_size = 0;
		for (int i = 0; i < num_buckets; i++)
			if ((int)buckets[i].size() > max_size)
				max_size = buckets[i].size();
		return max_size;
	}

	void merge(const hashtable<value_type, num_buckets> &m)
	{
		for (int i = 0; i < num_buckets; i++)
		{
			int s = (int)buckets[i].size();
			buckets[i].insert(buckets[i].end(), m.buckets[i].begin(), m.buckets[i].end());
			inplace_merge(buckets[i].begin(), buckets[i].begin() + s, buckets[i].end());
		}
		count += m.count;
	}

	value_type &operator[](int index)
	{
		int i = 0;
		for (; i < num_buckets && buckets[i].size() <= index; i++)
			index -= buckets[i].size();
		return buckets[i][index];
	}

	hashtable<value_type, num_buckets> &operator=(const hashtable<value_type, num_buckets> &copy)
	{
		for (int i = 0; i < num_buckets; i++)
			buckets[i] = copy.buckets[i];
		count = copy.count;
		return *this;
	}

};

#endif
