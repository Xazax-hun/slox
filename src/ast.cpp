#include "include/ast.h"

#include <ostream>
#include <utility>
#include <sstream>

std::string print(Index<Token> t, const ASTContext& c)
{
    return print(c.getToken(t));
}

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
    return parenthesize(::print(b->op, printer.c), printer.print(b->left), printer.print(b->right));
}

std::string ASTPrinter::ExprPrintVisitor::operator()(const Assign* a) const
{
    return parenthesize(std::string_view("="), ::print(a->name, printer.c), printer.print(a->value));
}

std::string ASTPrinter::ExprPrintVisitor::operator()(const Unary* u) const
{
    return parenthesize(::print(u->op, printer.c), printer.print(u->subExpr));
}

std::string ASTPrinter::ExprPrintVisitor::operator()(const Literal* l) const
{
    return ::print(l->value, printer.c);
}

std::string ASTPrinter::ExprPrintVisitor::operator()(const Grouping* l) const
{
    return parenthesize(std::string_view("group"), printer.print(l->subExpr));
}

std::string ASTPrinter::ExprPrintVisitor::operator()(const DeclRef* r) const
{
    return ::print(r->name, printer.c);
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
    return parenthesize(std::string_view("var"), ::print(s->name, printer.c), init);
}

std::string ASTPrinter::StmtPrintVisitor::operator()(const Block* s) const
{
    std::stringstream ss;
    ss << "(block";

    for (auto child : s->statements)
    {
        ss << " " << printer.print(child);
    }

    ss << ")";
    return std::move(ss).str();
}

std::string ASTPrinter::StmtPrintVisitor::operator()(const IfStatement* s) const
{
    std::string elseDump = s->elseBranch ? printer.print(*s->elseBranch) : "<NULL>";
    return parenthesize(std::string_view("if"), printer.print(s->condition),
        printer.print(s->thenBranch), elseDump);
}

std::string ASTPrinter::StmtPrintVisitor::operator()(const WhileStatement* s) const
{
    return parenthesize(std::string_view("while"), printer.print(s->condition),
        printer.print(s->body));
}
