////////////////////////////////////////////////////////////////////////////////
// taskwarrior - a command line task list manager.
//
// Copyright 2006 - 2011, Paul Beckingham, Federico Hernandez.
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the
//
//     Free Software Foundation, Inc.,
//     51 Franklin Street, Fifth Floor,
//     Boston, MA
//     02110-1301
//     USA
//
////////////////////////////////////////////////////////////////////////////////

#include <iostream> // TODO Remove.
#include <sstream>
#include <Context.h>
#include <Lexer.h>
#include <Expression.h>

extern Context context;

////////////////////////////////////////////////////////////////////////////////
// Perform all the necessary steps prior to an eval call.
Expression::Expression (Arguments& arguments)
: _original (arguments)
{
  expand_sequence ();
  to_infix ();
  expand_expression ();
  to_postfix ();
}

////////////////////////////////////////////////////////////////////////////////
Expression::~Expression ()
{
}

////////////////////////////////////////////////////////////////////////////////
bool Expression::eval (Task& task)
{
  // TODO Duplicate the _postfix vector as the operating stack.
  // TODO ...

  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Convert:  1,3-5,00000000-0000-0000-0000-000000000000
//
// To:       (id=1 or (id>=3 and id<=5) or
//            uuid="00000000-0000-0000-0000-000000000000")
void Expression::expand_sequence ()
{
  Arguments temp;
  _sequenced.clear ();

  // Extract all the components of a sequence.
  std::vector <int> ids;
  std::vector <std::string> uuids;
  std::vector <std::pair <std::string, std::string> >::iterator arg;
  for (arg = _original.begin (); arg != _original.end (); ++arg)
  {
    if (arg->second == "id")
      Arguments::extract_id (arg->first, ids);

    else if (arg->second == "uuid")
      Arguments::extract_uuid (arg->first, uuids);
  }

  // If there is no sequence, we're done.
  if (ids.size () == 0 && uuids.size () == 0)
    return;

  // Construct the algebraic form.
  std::stringstream sequence;
  sequence << "(";
  for (unsigned int i = 0; i < ids.size (); ++i)
  {
    if (i)
      sequence << " or ";

    sequence << "id=" << ids[i];
  }

  if (uuids.size ())
  {
    sequence << " or ";
    for (unsigned int i = 0; i < uuids.size (); ++i)
    {
      if (i)
        sequence << " or ";

      sequence << "uuid=\"" << uuids[i] << "\"";
    }
  }

  sequence << ")";
  std::cout << "# sequence '" << sequence.str () << "'\n";

  // Copy everything up to the first id/uuid.
  for (arg = _original.begin (); arg != _original.end (); ++arg)
  {
    if (arg->second == "id" || arg->second == "uuid")
      break;

    temp.push_back (*arg);
  }

  // Now insert the new sequence expression.
  temp.push_back (std::make_pair (sequence.str (), "exp"));

  // Now copy everything after the last id/uuid.
  bool found_id = false;
  for (arg = _original.begin (); arg != _original.end (); ++arg)
  {
    if (arg->second == "id" || arg->second == "uuid")
      found_id = true;

    else if (found_id)
      temp.push_back (*arg);
  }

  _sequenced.swap (temp);
  _sequenced.dump ("Expression::expand_sequence");
}

////////////////////////////////////////////////////////////////////////////////
// Convert:  +with -without
//
// To:       tags ~ with
//           tags !~ without
void Expression::expand_tag (const std::string& input)
{
   char type;
   std::string value;
   Arguments::extract_tag (input, type, value);

   _infix.push_back (std::make_pair ("tags", "dom"));
   _infix.push_back (std::make_pair (type == '+' ? "~" : "!~", "op"));
   _infix.push_back (std::make_pair (value, "exp"));
}

////////////////////////////////////////////////////////////////////////////////
// Convert:  <name>[:=]<value>
//
// To:       <name> = lex<value>
void Expression::expand_attr (const std::string& input)
{
  // TODO Should canonicalize 'name'.
  std::string name;
  std::string value;
  Arguments::extract_attr (input, name, value);

  _infix.push_back (std::make_pair (name, "dom"));
  _infix.push_back (std::make_pair ("=", "op"));
  _infix.push_back (std::make_pair (value, "exp"));
}

////////////////////////////////////////////////////////////////////////////////
// Convert:  <name>.<mod>[:=]<value>
//
// To:       <name> <op> lex<value>
void Expression::expand_attmod (const std::string& input)
{
  // TODO Should canonicalize 'name'.
  std::string name;
  // TODO Should canonicalize 'mod'.
  std::string mod;
  std::string value;
  std::string sense;
  Arguments::extract_attmod (input, name, mod, value, sense);

  if (mod == "before" || mod == "under" || mod == "below")
  {
    _infix.push_back (std::make_pair (name, "dom"));
    _infix.push_back (std::make_pair ("<", "op"));
    _infix.push_back (std::make_pair (value, "exp"));
  }
  else if (mod == "after" || mod == "over" || mod == "above")
  {
    _infix.push_back (std::make_pair (name, "dom"));
    _infix.push_back (std::make_pair (">", "op"));
    _infix.push_back (std::make_pair (value, "exp"));
  }
  else if (mod == "none")
  {
    _infix.push_back (std::make_pair (name, "dom"));
    _infix.push_back (std::make_pair ("==", "op"));
    _infix.push_back (std::make_pair ("\"\"", "exp"));
  }
  else if (mod == "any")
  {
    _infix.push_back (std::make_pair (name, "dom"));
    _infix.push_back (std::make_pair ("!=", "op"));
    _infix.push_back (std::make_pair ("\"\"", "exp"));
  }
  else if (mod == "is" || mod == "equals")
  {
    _infix.push_back (std::make_pair (name, "dom"));
    _infix.push_back (std::make_pair ("=", "op"));
    _infix.push_back (std::make_pair (value, "exp"));
  }
  else if (mod == "isnt" || mod == "not")
  {
    _infix.push_back (std::make_pair (name, "dom"));
    _infix.push_back (std::make_pair ("!=", "op"));
    _infix.push_back (std::make_pair (value, "exp"));
  }
  else if (mod == "has" || mod == "contains")
  {
    _infix.push_back (std::make_pair (name, "dom"));
    _infix.push_back (std::make_pair ("~", "op"));
    _infix.push_back (std::make_pair (value, "exp"));
  }
  else if (mod == "hasnt")
  {
    _infix.push_back (std::make_pair (name, "dom"));
    _infix.push_back (std::make_pair ("!~", "op"));
    _infix.push_back (std::make_pair (value, "exp"));
  }
  else if (mod == "startswith" || mod == "left")
  {
    // TODO ?
  }
  else if (mod == "endswith" || mod == "right")
  {
    // TODO ?
  }
  else if (mod == "word")
  {
    // TODO ?
  }
  else if (mod == "noword")
  {
    // TODO ?
  }
}

////////////////////////////////////////////////////////////////////////////////
// Convert:  <word>
//
// To:       description ~ <word>
void Expression::expand_word (const std::string& input)
{
  _infix.push_back (std::make_pair ("description", "dom"));
  _infix.push_back (std::make_pair ("~", "op"));
  _infix.push_back (std::make_pair (input, "exp"));
}

////////////////////////////////////////////////////////////////////////////////
// Convert:  /<pattern>/
//
// To:       description ~ <pattern>
void Expression::expand_pattern (const std::string& input)
{
  std::string value;
  Arguments::extract_pattern (input, value);

  _infix.push_back (std::make_pair ("description", "dom"));
  _infix.push_back (std::make_pair ("~", "op"));
  _infix.push_back (std::make_pair (value, "exp"));
}

////////////////////////////////////////////////////////////////////////////////
// Convert:  <exp>
//
// To:       lex<exp>
void Expression::expand_expression ()
{
  Arguments temp;

  std::vector <std::pair <std::string, std::string> >::iterator arg;
  for (arg = _infix.begin (); arg != _infix.end (); ++arg)
  {
    if (arg->second == "exp")
    {
      Lexer lexer (arg->first);
      lexer.skipWhitespace (true);
      lexer.coalesceAlpha (true);
      lexer.coalesceDigits (true);
      lexer.coalesceQuoted (true);

      std::vector <std::string> tokens;
      lexer.tokenize (tokens);

      std::vector <std::string>::iterator token;
      for (token = tokens.begin (); token != tokens.end (); ++token)
      {
        if (_infix.is_operator (*token))
          temp.push_back (std::make_pair (*token, "op"));
        else
          temp.push_back (std::make_pair (*token, "dom"));
      }
    }
    else
      temp.push_back (*arg);
  }

  _infix.swap (temp);
  _infix.dump ("Expression::expand_expression");
}

////////////////////////////////////////////////////////////////////////////////
// Inserts the 'and' operator by default between terms that are not separated by
// at least one operator.
//
// Converts:  <term1>     <term2> <op> <exp>
// to:        <term1> and <term2> <op> <token> <token> <token>
//
//
//
// Rules:
//   1. Two adjacent non-operator arguments have an 'and' inserted between them.
//   2. Any argument of type "exp" is lexed and replaced by tokens.
//
void Expression::to_infix ()
{
  _infix.clear ();

  bool new_style = is_new_style ();

  std::string value;
  std::string previous = "op";
  std::vector <std::pair <std::string, std::string> >::iterator arg;
  for (arg = _sequenced.begin (); arg != _sequenced.end (); ++arg)
  {
    // Old-style filters need 'and' conjunctions.
    if (!new_style          &&
        previous    != "op" &&
        arg->second != "op")
    {
      _infix.push_back (std::make_pair ("and", "op"));
    }

    // Upgrade all arguments to new-style.
    // ID & UUID sequence has already been converted.
    if (arg->second == "id" ||
        arg->second == "uuid")
      ; // NOP.

    else if (arg->second == "tag")
      expand_tag (arg->first);

    else if (arg->second == "pattern")
      expand_pattern (arg->first);

    else if (arg->second == "attribute")
      expand_attr (arg->first);

    else if (arg->second == "attmod")
      expand_attmod (arg->first);

    else if (arg->second == "word")
      expand_word (arg->first);

    // Expressions will be converted later.
    else if (arg->second == "exp")
      _infix.push_back (*arg);

    else
      throw std::string ("Error: unrecognized argument category '") + arg->second + "'";

    previous = arg->second;
  }

  _infix.dump ("Expression::toInfix");
}

////////////////////////////////////////////////////////////////////////////////
// Dijkstra Shunting Algorithm.
//
//   While there are tokens to be read:
//     Read a token.
//     If the token is a number, then add it to the output queue.
//     If the token is a function token, then push it onto the stack.
//     If the token is a function argument separator (e.g., a comma):
//       Until the token at the top of the stack is a left parenthesis, pop
//       operators off the stack onto the output queue. If no left parentheses
//       are encountered, either the separator was misplaced or parentheses were
//       mismatched.
//     If the token is an operator, o1, then:
//       while there is an operator token, o2, at the top of the stack, and
//             either o1 is left-associative and its precedence is less than or
//             equal to that of o2,
//             or o1 is right-associative and its precedence is less than that
//             of o2,
//         pop o2 off the stack, onto the output queue;
//       push o1 onto the stack.
//     If the token is a left parenthesis, then push it onto the stack.
//     If the token is a right parenthesis:
//       Until the token at the top of the stack is a left parenthesis, pop
//       operators off the stack onto the output queue.
//       Pop the left parenthesis from the stack, but not onto the output queue.
//       If the token at the top of the stack is a function token, pop it onto
//       the output queue.
//       If the stack runs out without finding a left parenthesis, then there
//       are mismatched parentheses.
//   When there are no more tokens to read:
//     While there are still operator tokens in the stack:
//       If the operator token on the top of the stack is a parenthesis, then
//       there are mismatched parentheses.
//       Pop the operator onto the output queue.
//   Exit.
//
void Expression::to_postfix ()
{
  _postfix.clear ();

  _postfix.dump ("Expression::toPostfix");
}

////////////////////////////////////////////////////////////////////////////////
// Test whether the _original arguments are old style or new style.
//
// Old style:  no single argument corresponds to an operator, ie no 'and', 'or',
//             etc.
//
// New style:  at least one argument that is an operator.
//
bool Expression::is_new_style ()
{
  std::vector <std::pair <std::string, std::string> >::iterator arg;
  for (arg = _original.begin (); arg != _original.end (); ++arg)
    if (Arguments::is_operator (arg->first))
      return true;

  return false;
}

////////////////////////////////////////////////////////////////////////////////
// TODO Remove?
void Expression::dump (const std::string& label)
{
}

////////////////////////////////////////////////////////////////////////////////
