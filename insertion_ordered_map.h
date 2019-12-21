#ifndef _INSERTION_ORDERED_MAP_H
#define _INSERTION_ORDERED_MAP_H

#include <unordered_map>
#include <list>
#include <string>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <stdexcept>

class lookup_error : std::exception { };

template <class K, class V, class Hash = std::hash<K>>
class insertion_ordered_map {

private:
    class map_structure;
    std::shared_ptr<map_structure> map;

public:

    insertion_ordered_map() :
            map(std::make_shared<map_structure>()) {}

    insertion_ordered_map(insertion_ordered_map const &other) :
            map(std::make_shared<map_structure>(*(other.map))) {}

    insertion_ordered_map(insertion_ordered_map &&other) :
            map(other.map)
    {
        other.map = nullptr;
    }

    insertion_ordered_map &operator=(insertion_ordered_map other) {
        // points to the very same structure
        map = other.map;

        return *this;
    }

    bool insert(K const &k, V const &v) {
        return map->insert(k, v).second;
    }

    void erase(K const &k) {
        map->erase(k);
    }

    void merge(insertion_ordered_map const &other) {
        if(&other == this) return;

        map->merge(*(other.map));
    }

    V &at(K const &k) {
        return map->at(k);
    }

    V const &at(K const &k) const {
        return map->at(k);
    }

    V &operator[](K const &k) {
        return (*map)[k];
    }

    size_t size() const noexcept {
        return map->size();
    }

    bool empty() const noexcept {
        return size() == 0;
    }

    void clear() {
        map->clear();
    }

    bool contains(K const &k) const {
        return map->contains(k);
    }

    // Iterators //

    using iterator = typename map_structure::iterator;

    iterator begin() const {
        return map->begin();
    }

    iterator end() const {
        return map->end();
    }
};


template <class K, class V, class Hash>
class insertion_ordered_map<K, V, Hash>::map_structure {
private:
    struct structure;
    using map_iterator = typename structure::maptype::iterator;
    using insertions_iterator = typename structure::keylisttype::iterator;
    using insertions_const_iterator = typename structure::keylisttype::const_iterator;

    std::shared_ptr<structure> data;

    void copy() { // strong
        data = std::make_shared<structure>(*data);
    }

    void copy_on_write() { // strong
        if(data.use_count() > 1)
            copy();
    }

public:

    // Iterator //
    class iterator {
    private:
        using pointer = std::shared_ptr<map_structure const>;
        using pair_type = std::pair<K const&, V const&>;

        insertions_const_iterator iter;
        const pointer ptr;

    public:

        iterator(const iterator& other) :
            iter(other.iter),
            ptr(other.ptr)
        {}

        iterator(insertions_const_iterator it, const map_structure* const iterated) :
            iter(it),
            ptr(iterated)
        {}

        iterator operator++() {
            iter++;
            return *this;
        }

        const pair_type operator*() const {
            return make_pair(*iter, ptr->at(*iter));
        }

        bool operator==(const iterator& rhs) const { return iter == rhs.iter; }
        bool operator!=(const iterator& rhs) const { return iter != rhs.iter; }

        const std::unique_ptr<pair_type> operator->() const {
            return std::make_unique<pair_type>(*iter, ptr->at(*iter));
        }
    };

    iterator begin() const {
        auto it = data->insertions.begin();
        return iterator(it, this);
    }

    iterator end() const {
        auto it = data->insertions.end();
        return iterator(it, this);
    }
    //

    map_structure() :
            data(std::make_shared<structure>()) {}

    map_structure(map_structure const &other) :
            data(other.data)
    {
        if(!data->non_const_refs_given.empty())
            copy();
    }

    std::pair<map_iterator, bool> insert(K const &k, V const &v) {
        copy_on_write();

        insertions_iterator iter = data->insertions.insert(data->insertions.end(), k);

        try {
            std::pair<map_iterator, bool> insertion_result =
                    data->mappings.insert({k, structure::mapValue(iter, v)});

            if(insertion_result.second == false) {
                data->insertions.erase(insertion_result.first->second.iter);
                insertion_result.first->second.iter = iter;
            }

            return insertion_result;
        }
        catch (...) {
            data->insertions.erase(iter);
            throw;
        }
    }

    void erase(K const &k) {
        if(contains(k) == false) throw lookup_error();
        copy_on_write();                                // doesn't modify the logical state

        map_iterator iter = data->mappings.find(k);    // doesn't modify the logical state
        data->non_const_refs_given.erase(k);           // strong

        data->insertions.erase(iter->second.iter);     // no-throw
        data->mappings.erase(iter);                    // no-throw
    }

    void merge(map_structure const &other) {
        if(&other == this) return;

        std::shared_ptr<structure> backup_data = data;

        try {
            copy();

            for(auto k: other.data->insertions)
                insert(k, other.at(k));
        }
        catch (...) {
            data = backup_data;
            throw;
        }
    }

    V &at(K const &k) {
        if(contains(k) == false) throw lookup_error();
        copy_on_write();

        auto insertion_result = data->non_const_refs_given.insert(k);

        try {
            return data->mappings.at(k).value;
        }
        catch (...) {
            if(insertion_result.second == true) {
                data->non_const_refs_given.erase(insertion_result.first);
            }
            throw;
        }
    }

    V const &at(K const &k) const {
        if(contains(k) == false) throw lookup_error();

        return data->mappings.at(k).value;
    }

    V &operator[](K const &k) {
        auto insertion_result = insert(k, V());

        try {
            return at(k);
        }
        catch (...) {
            if(insertion_result.second == true) {
                data->mappings.erase(insertion_result.first);
                data->insertions.pop_back();
            }
            throw;
        }
    }

    size_t size() const noexcept {
        return data->insertions.size();
    }

    void clear() {
        copy_on_write();                    // strong

        data->mappings.clear();             // no-throw
        data->insertions.clear();           // no-throw
        data->non_const_refs_given.clear(); // no-throw
    }

    bool contains(K const &k) const {
        return data->mappings.count(k) > 0;
    }


};

template <class K, class V, class Hash>
struct insertion_ordered_map<K, V, Hash>::map_structure::structure {
    using keylisttype = std::list<const K>;

    struct mapvaluetype {
        typename keylisttype::iterator iter;
        V value;

        mapvaluetype () {}

        mapvaluetype(mapvaluetype const &other) :
                iter(other.iter),
                value(other.value) {}

        mapvaluetype(typename keylisttype::iterator iter, V value) :
                iter(iter),
                value(value) {}
    };

    using maptype = std::unordered_map<K, mapvaluetype, Hash>;
    using keysettype = std::unordered_set<K, Hash>;

    /*
     * As per cppreference.com:
     * If an insertion occurs and results in a rehashing of the container,
     * all iterators are invalidated. Otherwise iterators are not affected.
     * References are not invalidated. Rehashing occurs only if the new
     * number of elements is greater than max_load_factor()*bucket_count().
     */

    maptype mappings;
    keylisttype insertions;
    keysettype non_const_refs_given;

    structure() :
            mappings(),
            insertions(),
            non_const_refs_given() {};

    structure(structure const &other) :
            mappings(other.mappings),
            insertions(other.insertions),
            non_const_refs_given()
    {
        for(auto it = insertions.begin(); it != insertions.end(); it++) {
            mappings[*it].iter = it;
        }
    };

    static mapvaluetype mapValue(typename keylisttype::iterator iter, V value) {
        return mapvaluetype(iter, value);
    }

};

#endif
