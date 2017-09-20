//
// Copyright (C) 2017, Lucas Ondel
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

//
// This class implements a Finite State Acceptor (FSA) representing
// the likelihood of some observed data given a set a model (GMM,
// neural networks, ...). Internally, the FST is stored as a matrix
// (NxK) where N is the number of observed samples and K is the number
// of "state" of the model (not to be confused with the FSA's states).
//


#ifndef OBS_FST_H
#define OBS_FST_H

#include <iostream>
#include <fst/fst.h>
#include <fst/fstlib.h>
#include <fst/script/fst-class.h>
#include <string>
#include <vector>

using namespace std;
using namespace fst;
using namespace fst::script;

template <class Arc>
class ObsFst: public ExpandedFst<Arc> {
    public:
        using Weight = typename Arc::Weight;
        using StateId = typename Arc::StateId;

        ObsFst(const double* data, const int n_frames, const int n_model_states,
               SymbolTable* isyms, SymbolTable* osyms)
            : type_("obsfst"),
            data_(data),
            n_frames_(n_frames),
            n_model_states_(n_model_states),
            isyms_(isyms->Copy()),
            osyms_(osyms->Copy()),
            final_state_(n_frames),
            properties_(kAcceptor | kIDeterministic | kODeterministic | kNoEpsilons
                | kWeighted | kAcyclic | kAccessible | kCoAccessible | kNotString),
            arcs_buffer_(n_model_states) {}

        virtual ~ObsFst() {}

        // Initial state.
        virtual StateId Start() const { return 0; }

        // State's final weight.
        virtual Weight Final(StateId id) const {
            if (id == this->final_state_) {
                return Weight::One();
            }
            return Weight::Zero();
        }

        // State count.
        virtual StateId NumStates() const { return this->n_frames_ + 1; }

        // State's arc count.
        virtual size_t NumArcs(StateId id) const {
            if (id == this->final_state_) {
                return 0;
            }
            return this->n_model_states_;
        }

        // State's input epsilon count.
        virtual size_t NumInputEpsilons(StateId id) const { return 0; }

        // State's output epsilon count.
        virtual size_t NumOutputEpsilons(StateId id) const { return 0; }

        // Property bits.
        virtual uint64 Properties(uint64 mask, bool test) const {
            if (test) {
                uint64 knownprops, testprops = TestProperties<Arc>(
                    *this, mask, &knownprops);
                return testprops & mask;
            } else {
                return this->properties_ & mask;
            }
        }

        // FST type name.
        virtual const string& Type() const { return this->type_; }

        // Shallow copy the FST. We ignore the safe argument.
        virtual ObsFst<Arc>* Copy(bool stafe=false) const {
            return new ObsFst<Arc>(this->data_, this->n_frames_,
                this->n_model_states_, this->isyms_.get(), this->osyms_.get());
        }

        // Returns input label symbol table or nullptr if not specified.
        virtual const SymbolTable* InputSymbols() const { return this->isyms_.get(); }

        // Returns output label symbol table or nullptr if not specified.
        virtual const SymbolTable* OutputSymbols() const { return this->osyms_.get(); }

        // Generic state iterator construction. We use the default state iterator.
        virtual void InitStateIterator(StateIteratorData<Arc>* data) const {
            data->nstates = this->n_frames_ + 1;
        }

        // Generic arc iterator construction.
        virtual void InitArcIterator(StateId id, ArcIteratorData<Arc>* data) const {
            if (id == this->final_state_) {
                data->narcs = 0;
            } else {
                for (uint32 i = 0; i < this->n_model_states_; ++i) {
                    this->arcs_buffer_[i].ilabel = i + 1;
                    this->arcs_buffer_[i].olabel = i + 1;
                    this->arcs_buffer_[i].nextstate = id + 1;
                    this->arcs_buffer_[i].weight = \
                        Weight(-this->data_[(id * this->n_model_states_) + i]);
                }
                data->arcs = &(this->arcs_buffer_[0]);
                data->narcs = this->arcs_buffer_.size();
            }
        }

    protected:
        string type_;
        uint64 properties_;
        StdArc::StateId final_state_;
        const double* data_;
        const int n_frames_;
        const int n_model_states_;
        unique_ptr<SymbolTable> isyms_;
        unique_ptr<SymbolTable> osyms_;
        mutable vector<Arc> arcs_buffer_;
};


FstClass*
CreateObsFst(const double* data, const int n_frames, const int n_model_states,
             SymbolTable* isyms, SymbolTable* osyms) {

    ObsFst<LogArc> fst(data, n_frames, n_model_states, isyms, osyms);
    return new FstClass(fst);
}


#endif

