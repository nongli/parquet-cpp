// Copyright 2012 Cloudera Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "parquet/schema.h"

#include "parquet/parquet.h"

using namespace boost;
using namespace parquet;
using namespace std;

namespace parquet_cpp {

Projection::Projection(const vector<vector<string> >& cols)
  : cols_by_name_(cols) {
}

Schema::Element::Element(const SchemaElement& s, Element* parent) :
  parquet_schema_(s),
  parent_(parent),
  max_def_level_(parent == NULL ? 0 : parent->max_def_level() +
      (s.repetition_type != FieldRepetitionType::REQUIRED)),
  max_rep_level_(parent == NULL ? 0 : parent->max_rep_level() +
      (s.repetition_type == FieldRepetitionType::REPEATED)),
  index_in_parent_(-1),
  projected_(true),
  projected_index_in_parent_(-1) {
}

string Schema::Element::ToString(const string& prefix, bool projected_only) const {
  if (projected_only && !projected_) return "";

  stringstream ss;
  ss << prefix;
  if (num_children() == 0) {
    ss << index_in_parent_ << ": " << PrintRepetitionType(parquet_schema_) << " "
       << PrintType(parquet_schema_) << " " << name() << ";" << endl;
  } else {
    if (parent_ == NULL) {
      ss << "{" << endl;
    } else {
      ss << index_in_parent_ << ": "
         << PrintRepetitionType(parquet_schema_) << " struct " << name()
         << " {" << endl;
    }
    for (int i = 0; i < num_children(); ++i) {
      ss << children_[i]->ToString(prefix + "  ", projected_only);
    }
    ss << prefix << "};" << endl;
  }
  return ss.str();
}

void Schema::Element::Compile() {
  if (!is_root()) {
    index_in_parent_ = parent_->IndexOf(name(), false);
    if (projected_) {
      projected_index_in_parent_ = parent_->IndexOf(name(), true);
    }
  }
  for (int i = 0; i < children_.size(); ++i) {
    children_[i]->Compile();
  }

  projected_children_.clear();
  for (int i = 0; i < children_.size(); ++i) {
    if (children_[i]->projected_) projected_children_.push_back(children_[i].get());
  }

  ordinal_path_.clear();
  schema_path_.clear();
  string_path_.clear();
  projected_ordinal_path_.clear();

  const Schema::Element* current = this;
  while (!current->is_root()) {
    ordinal_path_.insert(ordinal_path_.begin(), current->index_in_parent());
    schema_path_.insert(schema_path_.begin(), current);
    string_path_.insert(string_path_.begin(), current->name());
    if (projected_) {
      projected_ordinal_path_.insert(
          projected_ordinal_path_.begin(), current->projected_index_in_parent());
    }
    current = current->parent();
  }

  stringstream ss;
  for (int i = 0; i < string_path_.size(); ++i) {
    if (i == 0) {
      ss << string_path_[i];
    } else {
      ss << "." << string_path_[i];
    }
  }
  full_name_ = ss.str();
}

void Schema::Element::ClearProjection() {
  projected_ = false;
  for (int i = 0; i < children_.size(); ++i) children_[i]->ClearProjection();
}

int Schema::Element::IndexOf(const string& name, bool projected_only) const {
  int idx = 0;
  for (int i = 0; i < children_.size(); ++i) {
    if (projected_only && !children_[i]->projected_) continue;
    if (children_[i]->name() == name) return idx;
    ++idx;
  }
  throw ParquetException("Invalid child.");
}

void Schema::Parse(const vector<SchemaElement>& nodes, Element* parent, int* idx) {
  int cur_idx = *idx;
  for (int i = 0; i < nodes[cur_idx].num_children; ++i) {
    ++*idx;
    shared_ptr<Element> child(new Element(nodes[*idx], parent));
    parent->children_.push_back(child);
    Parse(nodes, child.get(), idx);
  }
  if (nodes[cur_idx].num_children == 0) {
    max_def_level_ = std::max(max_def_level_, parent->max_def_level());
    leaves_.push_back(parent);
    projected_leaves_.push_back(parent);
  }
}

void Schema::SetProjection(Projection* projection) {
  root_->ClearProjection();
  for (int i = 0; i < projection->cols_by_name_.size(); ++i) {
    Schema::Element* node = root_.get();
    node->projected_ = true;
    const vector<string>& path = projection->cols_by_name_[i];
    for (int j = 0; j < path.size(); ++j) {
      int idx = node->IndexOf(path[j], false);
      node = node->children_[idx].get();
      node->projected_ = true;
    }
  }
  root_->Compile();

  projected_leaves_.clear();
  for (int i = 0; i < leaves_.size(); ++i) {
    if (leaves_[i]->projected_) projected_leaves_.push_back(leaves_[i]);
  }
}

shared_ptr<Schema> Schema::FromParquet(const vector<SchemaElement>& nodes) {
  shared_ptr<Schema> schema(new Schema());
  if (nodes.size() == 0) {
    throw ParquetException("Invalid empty schema");
  }
  schema->root_.reset(new Element(nodes[0], NULL));
  int idx = 0;
  schema->Parse(nodes, schema->root_.get(), &idx);
  schema->root_->Compile();
  return schema;
}

}
