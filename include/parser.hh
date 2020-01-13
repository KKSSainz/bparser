/*
 * parser.hh
 *
 *  Created on: Jan 11, 2020
 *      Author: jb
 */


#ifndef INCLUDE_PARSER_HH_
#define INCLUDE_PARSER_HH_


#include "ast.hh"
#include "evaluator.hh"
#include "processor.hh"
#include "grammar.hh"
#include "expr.hh"


#include <map>
#include <memory>
#include <string>

namespace bparser {

/// @brief Parse a mathematical expression
///
/// This can parse and evaluate a mathematical expression for a given
/// symbol table using Boost.Spirit X3.  The templates of Boost.Spirit
/// are very expensive to parse and instantiate, which is why we hide
/// it behind an opaque pointer.
///
/// The drawback of this approach is that calls can no longer be
/// inlined and because the pointer crosses translation unit
/// boundaries, dereferencing it can also not be optimized out at
/// compile time.  We have to rely entirely on link-time optimization
/// which might be not as good.
///
/// The pointer to the implementation is a std::unique_ptr which makes
/// the class not copyable but only moveable.  Copying shouldn't be
/// required but is easy to implement.



class Parser {

	ast::operand ast;
	uint max_vec_size;
	std::map<std::string, expr::Array> symbols_;
	std::vector<std::string> free_variables;

public:
    /// @brief Constructor
    Parser(uint max_vec_size)
	: max_vec_size(max_vec_size)
	{}

    /// @brief Destructor
    ~Parser() {}

    /// @brief Parse the mathematical expression into an abstract syntax tree
    ///
    /// @param[in] expr The expression given as a std::string
    void parse(std::string const &expr) {
        ast::operand ast_;

        std::string::const_iterator first = expr.begin();
        std::string::const_iterator last = expr.end();

        boost::spirit::ascii::space_type space;
        bool r = qi::phrase_parse(
            first, last, grammar(), space,
            ast_);

        if (!r || first != last) {
            std::string rest(first, last);
            throw std::runtime_error("Parsing failed at " + rest); // NOLINT
        }

        ast = ast_;

        _optimize();
    }
    void _get_free_vars() {

    	//free_variables  = boost::apply_visitor(ast::get_variables(), ast);
    }

    void _optimize() {
    	//ast = boost::apply_visitor(ast::ConstantFolder(), ast);
    }

    /**
     * @brief Return names (undefined) variables in the expression.
     */
    std::vector<std::string> variables() {

    }

    /**
     * Set given name to be a variable of given shape with values at
     * given address 'variable_space'.
     *
     */
    void set_variable(std::string name, std::vector<uint> shape, double *variable_space) {
    	symbols_[name] = expr::Array::value(variable_space, max_vec_size, shape);
    }

    /**
     * Set given name to be a constant of given shape with flatten values
     * given by the 'const_value' vector.
     *
     */
    void set_constant(std::string name, std::vector<uint> shape, std::vector<double> const_value) {
    	symbols_[name] = expr::Array::constant(const_value, shape);
    }

    /// @brief Create processor of the expression from the AST.
    ///
    /// All variable names have to be set before this call.
//    Processor * make_processor() {
//        return boost::apply_visitor(ast::eval(st), ast);
//    }


};


} // namespace bparser




#endif /* INCLUDE_PARSER_HH_ */
