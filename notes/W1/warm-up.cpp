/* 20250826 W1L1*/

/* compile */
// bvim warm-up.cpp or gvim warm-up.cpp (only font size different)
// g++ -Wall -Werror -std=c++20 -o warm-up warm-up.cpp && ./warm-up

/* the six structures */

#include <vector> // mental model: dynamic array
#include <list> // mental model: doubly linked list
#include <set> // mental model: binary tree
#include <map> // mental model: binary tree of key-value pairs
#include <unordered_set> // mental model: hash table
#include <unordered_map> // mental model: hash table of key-value pairs

#include<chrono>
#include<iostream>

int main(int argc, char** argv) {

    // ways to turn off warnings without commenting out code:
    // std::list<uint32_t> list;
    // (void)list;
    // [[maybe_unused]]std::set<uint32_t> set;
    // std::map<uint32_t, uint32_t> map;
    // std::unordered_map<uint32_t, uint32_t> unordered_map;
    // std::unordered_set<uint32_t> unordered_set;

    uint32_t N = 10000000;

    // reserve space for vector to avoid reallocations
    {
        auto before = std::chrono::high_resolution_clock::now();
        std::vector<uint32_t> vector;
        vector.reserve(N); // avoid reallocations -> 2x speedup
        for (uint32_t i = 0; i < N; ++i) {
            vector.push_back(i);
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << "vector.push_back x " 
                  << N << " = " << std::chrono::duration<double>(after - before).count()
                  << "ms (with reserved)" << std::endl;
    }
    {
        auto before = std::chrono::high_resolution_clock::now();
        std::unordered_set<uint32_t> unordered_set;
        unordered_set.reserve(N); // avoid reallocations. better than .resize(), because //??v

        /*
        Why not resize(N)?
        resize(n) in an unordered container does not mean "allocate room for n elements" like in a vector. Instead:
        It changes the number of buckets directly (the hash table structure), not the number of stored elements.
        If you call resize(N) before inserting, you are actually forcing the container to rehash immediately into exactly N buckets. This might not be optimal, because the standard library typically chooses a bucket count based on prime numbers or powers of two to keep hash distribution efficient.
        Worse: resize(n) does not change the capacity of the elements, so you don’t gain the preallocation benefit that reserve() gives.
        */

        for (uint32_t i = 0; i < N; ++i) {
            unordered_set.insert(i);
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << "unordered_set.insert x " 
                  << N << " = " << std::chrono::duration<double>(after - before).count()
                  << "ms (with reserved)" << std::endl;
    }
    
    #if 0:
    // add a thing
    {
        auto before = std::chrono::high_resolution_clock::now();
        std::vector<uint32_t> vector;
        for (uint32_t i = 0; i < N; ++i) {
            vector.push_back(i); // copying the object into the structure // O(1)
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << "vector.push_back x " 
                  << N << " = " << std::chrono::duration<double>(after - before).count()
                  << "ms" << std::endl;
    }
    {
        auto before = std::chrono::high_resolution_clock::now();
        std::vector<uint32_t> vector;
        for (uint32_t i = 0; i < N; ++i) {
            vector.emplace_back(i); // constructing the object in place // O(1)
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << "vector.emplace_back x " 
                  << N << " = " << std::chrono::duration<double>(after - before).count()
                  << "ms" << std::endl;
    }
    {
        auto before = std::chrono::high_resolution_clock::now();
        std::vector<uint32_t> vector;
        for (uint32_t i = 0; i < N; ++i) {
            vector.insert(vector.end(), i); // inserting at the end of a vector //O(n)
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << "vector.insert x " 
                  << N << " = " << std::chrono::duration<double>(after - before).count()
                  << "ms" << std::endl;
    }

    {
        auto before = std::chrono::high_resolution_clock::now();
        std::list<uint32_t> list;
        for (uint32_t i = 0; i < N; ++i) {
            list.push_back(i); // copying the object into the structure // O(1)
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << "list.push_back x " 
                  << N << " = " << std::chrono::duration<double>(after - before).count()
                  << "ms" << std::endl;
    }
    {
        auto before = std::chrono::high_resolution_clock::now();
        std::list<uint32_t> list;
        for (uint32_t i = 0; i < N; ++i) {
            list.emplace_back(i); // constructing the object in place // O(1)
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << "list.emplace_back x " 
                  << N << " = " << std::chrono::duration<double>(after - before).count()
                  << "ms" << std::endl;
    }
    {
        auto before = std::chrono::high_resolution_clock::now();
        std::list<uint32_t> list;
        for (uint32_t i = 0; i < N; ++i) {
            list.insert(list.end(), i); // inserting at the end of a list // O(1)
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << "list.insert x " 
                  << N << " = " << std::chrono::duration<double>(after - before).count()
                  << "ms" << std::endl;
    }

    {
        auto before = std::chrono::high_resolution_clock::now();
        std::set<uint32_t> set;
        for (uint32_t i = 0; i < N; ++i) {
            set.insert(i); // inserting at the end of a set // O(log n)
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << "set.insert x " 
                  << N << " = " << std::chrono::duration<double>(after - before).count()
                  << "ms" << std::endl;
    }

    {
        auto before = std::chrono::high_resolution_clock::now();
        std::unordered_set<uint32_t> unordered_set;
        for (uint32_t i = 0; i < N; ++i) {
            unordered_set.insert(i); // inserting at the end of a unordered_set // O(1) amortized
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << "unordered_set.insert x " 
                  << N << " = " << std::chrono::duration<double>(after - before).count()
                  << "ms" << std::endl;
    }

    // find a thing
    uint32_t key = 466;
    {
        std::vector<uint32_t> vector; // assume already filled
        // 1
        vector.index_of(key);

        // 2
        auto found = std::find(vector.begin(), vector.end(), key);
        int foundIndex = found - vector.begin();

        // 3
        for (uint32_t i = 0; i < vector.size(); ++i) {
            if (vector[i] == key) {
                foundIndex = i;
                break;
            }
        }
    }
    {
        std::list<uint32_t> list;

        // 1
        auto found = std::find(vector.begin(), vector.end(), key); 
        // found - .begin() doesn't work for list //??v why

        /*
        std::vector provides random access iterators.
        These support arithmetic like it2 - it1, it + n, etc.
        So subtracting iterators gives you the distance (the element index) in O(1).

        std::list only provides bidirectional iterators (you can move forward/backward one step at a time, but not jump).
        They don’t support subtraction (-) or addition (+).
        To compute a position (distance from begin()), you must traverse step by step.s
        */

        // 2
        for (std::list<uint32_t>::const_iterator i = list.begin(); i != list.end(); ++i) {
            if (*i == key) {
                found = i;
                break;
            }

        }
    }
    {
        std::set<uint32_t> set;

        if (set.count(key) != 0) {
            // found
        }
    }
    {
        std::unordered_set<uint32_t> unordered_set;

        if (unordered_set.count(key) != 0) {
            // found
        }

        auto f = unirdered_set.find(key);
        if (f != unordered_set.end()) {
            // do something like remove it or w/e
        }
    }
    
    // remove a thing
    vector.pop_back(); // remove last element //O(1)
    vector.erase(vector.begin()); // remove first element //O(n)

    list.pop_back(); // remove last element //O(1)
    list.erase(list.begin()); // remove first element //O(1)
    // erase will be O(1) for list because it just needs to update the prev and next pointers that the parameter iterator points to
    // the only reason you use list: frequent insertions/deletions in the middle of the list

    set.erase(set.find(key)); // remove by key
    unordered_set.erase(unordered_set.find(key)); // remove by key

    // size of container / is it empty?
    vector.size();
    veector.empty();

    #endif
    return 0;
}

/**
Timing
issue 1:
timing is very off. moves at diff speed when at different window sizes
takes up too much cpu, because the game frame is much higher than display frame. display frame is usually 60Hz
run the game: ./dist/game
in main.cpp, SDL_GL_SetSwapInterval sets the swap interval too quick
when encounter a function like this, checkout its documentation

after code fix, run with node Maekfile.js && ./dist/game

issue 2:
player speed is set to 0.5f, so it moves at different speeds on different computers
fix: make speed relative to elapsed time instead of purely frame count
use std::chrono to measure elapsed time
if 100Hz, elapse = 0.01s, if 60Hz, elapse = 0.01667s
goal: set speed to playerSpeedAt60Hz * factor, where factor = elapsed / (1/60) = elapsed * 60

e.g. on faster computer, elapsed time is smaller, so background_fade and player moves less per frame
background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	constexpr float PlayerSpeed = 30.0f;
	if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	if (up.pressed) player_at.y += PlayerSpeed * elapsed;

issue 3:  if frames are taking a very long time to process, e.g. 1 second takes 2, 2 seocnds take 4, ...
			//lie to code, tell it to just run at 0.1f, lag to avoid spiral of death; 
			elapsed = std::min(0.1f, elapsed);
 */