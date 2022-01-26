
/**
 * @brief Simple class to avoid duplicate measurements.
 * 
 * @tparam T type of the value to deduplicate
 */
template<typename T> class Deduplicator {
 public:
  bool next(T value) {
    if (this->has_value_) {
      if (this->last_value_ == value)
        return false;
    }
    this->has_value_ = true;
    this->last_value_ = value;
    return true;
  }
  bool has_value() const { return this->has_value_; }

 protected:
  bool has_value_{false};
  T last_value_{};
};
