#pragma once

// This file is part of the Marian toolkit.
// Marian is copyright (c) 2016 Marcin Junczys-Dowmunt.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <map>

#include "definitions.h"
#include "chainable.h"
#include "node_operators.h"
#include "tensor.h"

namespace marian {

class ExpressionGraph;
typedef ExpressionGraph* ExpressionGraphPtr;

class Expr {
  public:
    Expr(ExpressionGraphPtr g, Chainable<Tensor>* chainable);
    
    Expr operator=(Tensor t) {
      pimpl_->setVal(t);
      return *this;
    }

    Tensor val();
    Tensor grad();

    ExpressionGraphPtr graph();
    
    ChainPtr node();
    operator ChainPtr();

    std::string Debug() const;

  private:
    ExpressionGraphPtr graph_;
    ChainPtr pimpl_;
};

class ExpressionGraph {
  public:
    ExpressionGraph() : stack_(new ChainableStack) {}
    
    void backprop(int batchSize) {
      forward(batchSize);
      backward();
    }
    
    void forward(int batchSize) {
      for(auto&& v : *stack_) {
        v->allocate(batchSize);
      }
      for(auto&& v : *stack_)
        v->forward();    
    }
    
    void backward() {
      for(auto&& v : *stack_)
        v->set_zero_adjoint();
    
      typedef typename ChainableStack::reverse_iterator It;
      stack_->back()->init_dependent();
      for(It it = stack_->rbegin(); it != stack_->rend(); ++it)
        (*it)->backward();
    }
    
    std::string graphviz() {
      std::stringstream ss;
      ss << "digraph ExpressionGraph {" << std::endl;
      ss << "rankdir=BT" << std::endl;
      typedef typename ChainableStack::reverse_iterator It;
      for(It it = stack_->rbegin(); it != stack_->rend(); ++it) {
        ss << (*it)->graphviz();
      }
      ss << "}" << std::endl;
      return ss.str();
    }
    
    /*********************************************************/

    /**
     * @brief Constructs a new node representing an input in an expression graph.
     *
     * This method records the input node in a list of input nodes,
     *    but does not attach the new input node to any existing expression graph.
     *
     * @param args           XXX Marcin, what are args here?
     *
     * @return a newly constructed input node
     */
    template <typename ...Args>
    inline Expr input(Args ...args) {
      Expr e(this, new InputNode(args...));
      inputs_.emplace_back(e);
      return e;
    }

    /**
     * @brief Constructs a new node representing a parameter in an expression graph.
     *
     * This method records the parameter node in a list of parameter nodes,
     *    but does not attach the new parameter node to any existing expression graph.
     *
     * @param args           XXX Marcin, what are args here?
     *
     * @return a newly constructed parameter node
     */
    template <typename ...Args>
    inline Expr param(Args ...args) {
      Expr e(this, new ParamNode(args...));
      params_.emplace_back(e);
      return e;
    }

    /**
     * @brief Constructs a new node representing a constant in an expression graph.
     *
     * This method does not attach the new constant node to any existing expression graph.
     *
     * @param args           XXX Marcin, what are args here?
     *
     * @return a newly constructed constant node
     */
    template <typename ...Args>
    inline Expr constant(Args ...args) {
      return Expr(this, new ConstantNode(args...));
    }

    /**
     * @brief Constructs a new node representing a constant (with value 1) in an expression graph.
     *
     * This method does not attach the new constant node to any existing expression graph.
     *
     * @param args           XXX Marcin, what are args here?
     *
     * @return a newly constructed constant node
     */
    template <typename ...Args>
    inline Expr ones(Args ...args) {
      return Expr(this, new ConstantNode(keywords::value=1, args...));
    }

    /**
     * @brief Constructs a new node representing a constant (with value 0) in an expression graph.
     *
     * This method does not attach the new constant node to any existing expression graph.
     *
     * @param args           XXX Marcin, what are args here?
     *
     * @return a newly constructed constant node
     */
    template <typename ...Args>
    inline Expr zeroes(Args ...args) {
      return Expr(this, new ConstantNode(keywords::value=0, args...));
    }
    
    /*********************************************************/
        
    ChainableStackPtr stack() {
      return stack_;
    }
    
    Expr& operator[](const std::string& name) {
      auto it = named_.find(name);
      UTIL_THROW_IF2(it == named_.end(), "No such named node in graph: " << name);
      return it->second;  
    }

    bool has_node(const std::string& name) const {
      return named_.count(name) > 0;
    }
    
    void add_named_node(Expr e, const std::string& name) {
      named_.emplace(name, e);
    }
    
    std::vector<Expr>& inputs() {
      return inputs_;
    }
    
    std::vector<Expr>& params() {
      return params_;
    }
    
  private:
    ChainableStackPtr stack_;
    
    std::map<std::string, Expr> named_;
    std::vector<Expr> params_;
    std::vector<Expr> inputs_;
};

}
