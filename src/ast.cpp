#include "include/ast.h"

#include <ostream>
#include <utility>

template<typename... T>
std::string parenthesize(T&&... strings)
{
    std::string result;
    result += "(";
    (((result += std::forward<T>(strings)) += " "), ...);
    result.pop_back();
    result += ")";
    return result;
}

std::string ASTPrinter::print(ExpressionIndex e) const
{
    auto node = c.getNode(e);
    return std::visit(visitor, node);
}

std::string ASTPrinter::PrintVisitor::operator()(const Binary* b) const
{
    return parenthesize(::print(b->op), printer.print(b->left), printer.print(b->right));
}

std::string ASTPrinter::PrintVisitor::operator()(const Unary* u) const
{
    return parenthesize(::print(u->op), printer.print(u->subExpr));
}

std::string ASTPrinter::PrintVisitor::operator()(const Literal* l) const
{
    return ::print(l->value);
}

std::string ASTPrinter::PrintVisitor::operator()(const Grouping* l) const
{
    return parenthesize(std::string_view("group"), printer.print(l->subExpr));
}
