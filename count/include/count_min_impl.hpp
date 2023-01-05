#ifndef COUNT_MIN_IMPL_HPP_
#define COUNT_MIN_IMPL_HPP_

#include "MurmurHash3.h"

namespace datasketches {

template<typename T>
count_min_sketch<T>::count_min_sketch(uint64_t num_hashes, uint64_t num_buckets, uint64_t seed):
num_hashes(num_hashes),
num_buckets(num_buckets),
seed(seed){
  sketch.resize(num_hashes*num_buckets) ;
  assert(num_buckets >= 3); // need epsilon at most 1.
};

template<typename T>
uint64_t count_min_sketch<T>::get_num_hashes() {
    return num_hashes ;
}

template<typename T>
uint64_t count_min_sketch<T>::get_num_buckets() {
    return num_buckets ;
}

template<typename T>
uint64_t count_min_sketch<T>::get_seed() {
    return seed ;
}

template<typename T>
std::vector<T> count_min_sketch<T>::get_sketch(){
  return sketch ;
}

template<typename T>
std::vector<uint64_t> count_min_sketch<T>::get_config(){
    std::vector<uint64_t> config(3) ;
    config = {get_num_hashes(), get_num_buckets(), get_seed()} ;
    return config ;
} ;

template<typename T>
double count_min_sketch<T>::get_relative_error(){
  return exp(1.0) / double(num_buckets) ;
}

template<typename T>
T count_min_sketch<T>::get_total_weight(){
  return total_weight ;
}

template<typename T>
uint64_t count_min_sketch<T>::suggest_num_buckets(double relative_error){
  /*
   * Function to help users select a number of buckets for a given error.
   * TODO: Change this when we use only power of 2 buckets.
   */
  //std::cout<< count_min_sketch::num_buckets << std::endl;
  if(relative_error < 0.){
    throw std::invalid_argument( "Relative error must be at least 0." );
  }
  return ceil(exp(1.0) / relative_error) ;
}

template<typename T>
uint64_t count_min_sketch<T>::suggest_num_hashes(double confidence){
  /*
   * Function to help users select a number of hashes for a given confidence
   * eg confidence is 1 - failure probability
   * failure probability == delta in the literature.
   * * TODO: Change this when update is improved
   */
  if(confidence < 0. || confidence > 1.0){
    throw std::invalid_argument( "Confidence must be between 0 and 1.0 (inclusive)." );
  }
  return ceil(log(1.0/(1.0 - confidence))) ;
}



template<typename T>
std::vector<uint64_t> count_min_sketch<T>::get_hashes(const void* item, size_t size){
  /*
   * Returns the hash locations for the input item.
   * Approach taken from
   * https://github.com/Claudenw/BloomFilter/wiki/Bloom-Filters----An-overview
   */
  uint64_t bucket_index ;
  std::vector<uint64_t> sketch_update_locations(num_hashes) ;
  HashState hashes;
  MurmurHash3_x64_128(item, size, seed, hashes); //


  uint64_t hash = hashes.h1 ;
  for(uint64_t hash_idx=0; hash_idx<num_hashes; ++hash_idx){
    hash += (hash_idx * hashes.h2) ;
    bucket_index = hash % num_buckets ;
    std::cout << " Bucket index: " << bucket_index << " HASH " << (hash_idx * num_buckets) + bucket_index << std::endl ;
    //sketch_update_locations.at(hash_idx) = (hash_idx * num_hashes) + bucket_index ;
    sketch_update_locations.at(hash_idx) = (hash_idx * num_buckets) + bucket_index ;
  }
  return sketch_update_locations ;
}

template<typename T>
T count_min_sketch<T>::get_estimate(uint64_t item) {return get_estimate(&item, sizeof(item));}

template<typename T>
T count_min_sketch<T>::get_estimate(const std::string& item) {
  if (item.empty()) return 0 ; // Empty strings are not inserted into the sketch.
  return get_estimate(item.c_str(), item.length());
}

template<typename T>
T count_min_sketch<T>::get_estimate(const void* item, size_t size){
  /*
   * Returns the estimated frequency of the item
   */
  std::vector<uint64_t> hash_locations = get_hashes(item, size) ;
  std::vector<T> estimates ;
  for (auto h: hash_locations){
    estimates.push_back(sketch[h]) ;
  }
  T result = *std::min_element(estimates.begin(), estimates.end());
  return result ;
}

template<typename T>
void count_min_sketch<T>::update(uint64_t item, T weight) {
  update(&item, sizeof(item), weight);
}

template<typename T>
void count_min_sketch<T>::update(uint64_t item) {
  std::cout << "item: " << item ;
  update(&item, sizeof(item), 1);
}

template<typename T>
void count_min_sketch<T>::update(const std::string& item, T weight) {
  if (item.empty()) return;
  update(item.c_str(), item.length(), weight);
}

template<typename T>
void count_min_sketch<T>::update(const std::string& item) {
  if (item.empty()) return;
  update(item.c_str(), item.length(), 1);
}

template<typename T>
void count_min_sketch<T>::update(const void* item, size_t size, T weight) {
  /*
   * Gets the value's hash locations and then increments the sketch in those
   * locations.
   */
  total_weight += weight ;
  std::vector<uint64_t> hash_locations = get_hashes(item, size) ;
  for (auto h: hash_locations){
    //std::cout << "hash: " << h << std::endl;
    sketch[h] += weight ;
  }
}


template<typename T>
T count_min_sketch<T>::get_upper_bound(uint64_t item) {return get_upper_bound(&item, sizeof(item));}

template<typename T>
T count_min_sketch<T>::get_upper_bound(const std::string& item) {
  if (item.empty()) return 0 ; // Empty strings are not inserted into the sketch.
  return get_upper_bound(item.c_str(), item.length());
}

template<typename T>
T count_min_sketch<T>::get_upper_bound(const void* item, size_t size){
  /*
   *
   */
  return get_estimate(item, size) + get_relative_error()*get_total_weight() ;
}


template<typename T>
T count_min_sketch<T>::get_lower_bound(uint64_t item) {return get_lower_bound(&item, sizeof(item));}

template<typename T>
T count_min_sketch<T>::get_lower_bound(const std::string& item) {
  if (item.empty()) return 0 ; // Empty strings are not inserted into the sketch.
  return get_lower_bound(item.c_str(), item.length());
}

template<typename T>
T count_min_sketch<T>::get_lower_bound(const void* item, size_t size){
  /*
   *
   */
  return get_estimate(item, size) ;
}

template<typename T>
void count_min_sketch<T>::merge(count_min_sketch<T> &other_sketch){
  /*
   * Merges this sketch into that sketch by elementwise summing of buckets
   */
  if(this == &other_sketch){
    throw std::invalid_argument( "Cannot merge a sketch with itself." );
  }

  bool same_sketch_config = (get_config() == other_sketch.get_config()) ;
  if(!same_sketch_config){
    throw std::invalid_argument( "Incompatible sketch config." );
  }

  std::vector<T> other_table = other_sketch.get_sketch() ;
  for(auto i=0 ; i<(num_hashes*num_buckets); ++i){
    sketch[i] += other_table[i] ;
  }
  total_weight += other_sketch.get_total_weight() ;

//  // Iterate through the table and increment
//  for(int i=0 ; i < num_hashes; i++){
//    for(int j = -0; j < num_buckets; j++){
//      table[i][j] += sketch.table[i][j] ;
//    }
//  }
//  total_weight += sketch.total_weight ;
}

} /* namespace datasketches */

#endif