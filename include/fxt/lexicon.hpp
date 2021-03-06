/*
 * Copyright 2018 The Fxt authors.
 *
 * For the full copyright and license information, please view the LICENSE file
 * that was distributed with this source code.
 */

#pragma once

#include "cereal/types/map.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

#include <map>
#include <string>
#include <utility>
#include <vector>

struct Counts {
  uint64_t document_count = 0;
  uint64_t term_count = 0;

  Counts() = default;
  Counts(uint64_t dc, uint64_t tc) : document_count(dc), term_count(tc) {}

  template <class Archive>
  void serialize(Archive &archive) {
    archive(document_count, term_count);
  }
};
using FieldCounts = std::map<uint64_t, Counts>;

class Term {
 private:
  Counts counts;
  FieldCounts field_counts;

 public:
  Term() = default;

  Term(const Counts &c, const FieldCounts &fc) : counts(c), field_counts(fc){};

  inline Counts get_counts() const { return counts; }

  inline FieldCounts get_field_counts() const { return field_counts; }

  inline uint64_t document_count() const { return counts.document_count; }

  inline uint64_t term_count() const { return counts.term_count; }

  inline uint64_t field_document_count(uint64_t field) const {
    auto it = field_counts.find(field);
    if (it == field_counts.end()) {
      return 0;
    }
    return it->second.document_count;
  }

  inline uint64_t field_term_count(uint64_t field) const {
    auto it = field_counts.find(field);
    if (it == field_counts.end()) {
      return 0;
    }
    return it->second.term_count;
  }

  template <class Archive>
  void serialize(Archive &archive) {
    archive(counts, field_counts);
  }
};

class Lexicon {
 public:
  inline static const size_t oov_id = 0;
  inline static const std::string oov_str = "xxoov";

  // Number of documents in the collection
  inline uint64_t document_count() const { return counts.document_count; }

  // Number of terms in the collection
  inline uint64_t term_count() const { return counts.term_count; }

  // Number of unique terms in the collection
  inline uint64_t length() const {
    if (terms.size() > 0) {
      // Subtract OOV term from length
      return terms.size() - 1;
    }
    return 0;
  }

  inline const Term &operator[](size_t pos) const { return terms[pos]; }
  inline Term &operator[](size_t pos) { return terms[pos]; }

  inline size_t term(const std::string &t) const {
    auto it = term_id.find(t);
    if (it != term_id.end()) {
      return it->second;
    }
    return oov_term();
  }

  inline size_t oov_term() const { return oov_id; }

  inline bool is_oov(size_t tid) { return tid == oov_term(); }

  inline const std::string &term(const size_t id) const { return id_term[id]; }

  void push_back(const std::string &t, const Counts &c, const FieldCounts &fc) {
    auto id = terms.size();
    term_id.insert(std::make_pair(t, id));
    id_term.emplace_back(t);
    Term term(c, fc);
    terms.push_back(term);
  }

  /**
   * FIXME #32 - Unify constructor behaviour with respect to OOV term setup.
   * Deserialization would just overwrite existing class members anyway right?
   *
   * The default constructor assumes that the `Lexicon` will be populated from
   * data on disk, which is why the `oov_str` is not configured, but it is in
   * the constructor that takes `Counts` as a parameter for
   * `Lexicon::Lexicon(Counts c)`.
   */
  Lexicon() = default;

  Lexicon(Counts c) : counts(c) { push_back(oov_str, {}, {}); }

  template <class Archive>
  void serialize(Archive &archive) {
    // FIXME - only need to store id_term, then term_id can be generated on
    // load. Needs changes to Cereal loading.
    archive(counts, terms, id_term, term_id);
  }

 private:
  Counts counts;
  std::vector<Term> terms;
  std::vector<std::string> id_term;
  std::map<std::string, size_t> term_id;
};
