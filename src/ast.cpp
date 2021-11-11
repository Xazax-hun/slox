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
    return std::visit(exprVisitor, node);
}

std::string ASTPrinter::print(StatementIndex e) const
{
    auto node = c.getNode(e);
    return std::visit(stmtVisitor, node);
}

std::string ASTPrinter::ExprPrintVisitor::operator()(const Binary* b) const
{
    return parenthesize(::print(b->op), printer.print(b->left), printer.print(b->right));
}

std::string ASTPrinter::ExprPrintVisitor::operator()(const Assign* a) const
{
    return parenthesize(std::string_view("="), ::print(a->name), printer.print(a->value));
}

std::string ASTPrinter::ExprPrintVisitor::operator()(const Unary* u) const
{
    return parenthesize(::print(u->op), printer.print(u->subExpr));
}

std::string ASTPrinter::ExprPrintVisitor::operator()(const Literal* l) const
{
    return ::print(l->value);
}

std::string ASTPrinter::ExprPrintVisitor::operator()(const Grouping* l) const
{
    return parenthesize(std::string_view("group"), printer.print(l->subExpr));
}

std::string ASTPrinter::ExprPrintVisitor::operator()(const DeclRef* r) const
{
    return ::print(r->name);
}

std::string ASTPrinter::StmtPrintVisitor::operator()(const PrintStatement* s) const
{
    return parenthesize(std::string_view("print"), printer.print(s->subExpr));
}

std::string ASTPrinter::StmtPrintVisitor::operator()(const ExprStatement* s) const
{
    return parenthesize(std::string_view("exprStmt"), printer.print(s->subExpr));
}

std::string ASTPrinter::StmtPrintVisitor::operator()(const VarDecl* s) const
{
    std::string init = s->init ? printer.print(*s->init) : "<NULL>";
    return parenthesize(std::string_view("var"), ::print(s->name), init);
}
