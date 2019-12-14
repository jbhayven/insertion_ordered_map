// DODAĆ GUARDY W WERSJI KOŃCOWEJ!

#include <unordered_map>
#include <list>
#include <unordered_set>
#include <memory>
#include <algorithm>

template <class K, class V, class Hash = std::hash<K>>
class insertion_ordered_map {
    
private:
    class map_structure {
    private:
        struct structure {
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
        
        std::shared_ptr<structure> data;
        
    public:
        map_structure() {
            data = std::make_shared<structure>;
        }
        
        map_structure(map_structure const &other) {
            data = other.data;
        }
        
        void copy() {
            data = std::make_shared<structure>(*data);
        }
    };
        
    std::shared_ptr<map_structure> map;

public:
    insertion_ordered_map() {
        map = std::make_shared<map_structure>();
    }
    
    insertion_ordered_map(insertion_ordered_map const &other) {
        // creates a new structure that shares its recourses with the old data
        map = std::make_shared<map_structure>(other.map->data);
        
        if(!map->data->non_const_refs_given.empty())
            map->copy();
    }
    
    insertion_ordered_map(insertion_ordered_map &&other) {
        // moves the pointer to the structure
        map = other.map;
        other.map = nullptr;
    }
    
    insertion_ordered_map &operator=(insertion_ordered_map other) {
        // points to the very same structure
        map = other.map;    
    }
    
    bool insert(K const &k, V const &v) {
        
    }

};

// DODAĆ GUARDY W WERSJI KOŃCOWEJ!
