// DODAĆ GUARDY W WERSJI KOŃCOWEJ!

#include <unordered_map>
#include <list>
#include <unordered_set>
#include <memory>
#include <algorithm>

template <class K, class V, class Hash = std::hash<K>>
class insertion_ordered_map {
    
private:
    class map_structure;
    std::shared_ptr<map_structure> map;

public:
    insertion_ordered_map() :
        map(std::make_shared<map_structure>()) {}
    
    insertion_ordered_map(insertion_ordered_map const &other) :
        map(std::make_shared<map_structure>(other)) {}
    
    insertion_ordered_map(insertion_ordered_map &&other) :
        map(other.map) 
    {
        other.map = nullptr;
    }
    
    insertion_ordered_map &operator=(insertion_ordered_map other) {
        // points to the very same structure
        map = other.map;    
    }
    
    bool insert(K const &k, V const &v) {
        return map->insert(k, v);
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
};


template <class K, class V, class Hash>
class insertion_ordered_map<K, V, Hash>::map_structure {
private:
    struct structure;
    
    std::shared_ptr<structure> data;
    
    void copy() { // strong
        data = std::make_shared<structure>(*data);
    }

    void copy_on_write() { // strong
        if(data.use_count > 1)
            copy();
    }
    
public:

    map_structure() :
        data(std::make_shared<structure>) {}
    
    map_structure(map_structure const &other) :
        data(other.data)
    {
        if(!data->non_const_refs_given.empty())
            copy();
    }
    
    bool insert(K const &k, V const &v) {
        std::shared_ptr<structure> old_data = data;
        
        return true;
    }
    
    V &operator[](K const &k) {
        if(!contains(k)) {
            insert(k, V());
        }
        
    }
    
    size_t size() const noexcept {
        return data->insertions->size();
    }

    void clear() {
        copy_on_write();                    // strong
        
        data->mappings.clear();             // noexcept
        data->insertions.clear();           // noexcept
        data->non_const_refs_given.clear(); // noexcept
    }
    
    bool contains(K const &k) const {
        return data->mappings.find(k) != data->mappings.end();
    }
};
        

template <class K, class V, class Hash>
struct insertion_ordered_map<K, V, Hash>::map_structure::structure {
    using keylisttype = std::list<K>;
    using maptype = std::unordered_map<K, std::pair<typename keylisttype::iterator, V> >;
    
    /* 
     * As per cppreference.com:
     * If an insertion occurs and results in a rehashing of the container, 
     * all iterators are invalidated. Otherwise iterators are not affected. 
     * References are not invalidated. Rehashing occurs only if the new 
     * number of elements is greater than max_load_factor()*bucket_count(). 
     */
    
    maptype mappings;
    keylisttype insertions;
    std::unordered_set<K> non_const_refs_given;
    
    structure() : 
        mappings(), 
        insertions(), 
        non_const_refs_given() {};
        
    structure(structure const &other) : 
        mappings(other.mappings), 
        insertions(other.insertions), 
        non_const_refs_given() 
    {
        for(auto it : insertions) {
            mappings[*it].first = it;
        }
    };
};

// DODAĆ GUARDY W WERSJI KOŃCOWEJ!
