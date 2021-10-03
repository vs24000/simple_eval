// ISO C++14 Standard

#include <iostream>
#include <string>
#include <vector>
#include <stack>

using namespace std;

const char* VERSION{ "0.1" };

enum class token_type {
	unknown,
	number,
	open_bracket,    // (
	close_bracket,   // )
	op_add,          // +
	op_sub,          // -
	op_mul,          // *
	op_div           // /
};

class token {
public:
	token_type type{};
	string text{};
	double number{};
	
	void clear() {
		type = {};
		text = {};
		number = {};
	}
};

using token_list = vector<token>;

class tokenizer {
private:
	string src;
	token_list tokens;
	bool error;
	bool is_value(char c);
	bool is_operator(char c);
	void verify_tokenlist();
public:
	tokenizer(const string &src_text);
	void parse();
	bool error_state();
	token_list get_tokens() const;
};

tokenizer::tokenizer(const string &src_text)
	: src{ src_text }, tokens{}, error{} {}

bool tokenizer::is_value(char c) {
	return isdigit(c) || (c == '.');
}
bool tokenizer::is_operator(char c) {
	return (c == '+') || (c == '-') || (c == '*') || (c == '/') ||
		(c == '(') || (c == ')');
}

void tokenizer::verify_tokenlist() {
	if (error) return;
	// verify if no parsing error
	int numbers_count = 0;
	int operations_count = 0;
	int open_brackets_count = 0;
	int close_brackets_count = 0;
	for (token t : tokens) {
		switch (t.type) {
			case token_type::number:
				++numbers_count;
				break;
			case token_type::open_bracket:
				++open_brackets_count;
				break;
			case token_type::close_bracket:
				++close_brackets_count;
				break;
			case token_type::op_add:
			case token_type::op_sub:
			case token_type::op_mul:
			case token_type::op_div:
				++operations_count;
				break;
			default:
				error = true;
				break;
		}
	}
	if (open_brackets_count != close_brackets_count) error = true;
	if (numbers_count != operations_count + 1) error = true;
}

void tokenizer::parse() {
	token t{};

	// read tokens
	for (char rune : src) {
		if (isspace(rune)) continue;
		if (is_value(rune)) {
			t.type = token_type::number;
			t.text += rune;
			continue;
		}
		if (is_operator(rune)) {
			if (t.type != token_type::unknown) {
				if(t.type == token_type::number)
					t.number = atof(t.text.c_str());
				tokens.push_back(t);
			}
			t.clear();
			if (rune == '+') t.type = token_type::op_add;
			if (rune == '-') t.type = token_type::op_sub;
			if (rune == '*') t.type = token_type::op_mul;
			if (rune == '/') t.type = token_type::op_div;
			if (rune == '(') t.type = token_type::open_bracket;
			if (rune == ')') t.type = token_type::close_bracket;
			t.text.append(1, rune);
			tokens.push_back(t);
			t.clear();
			continue;
		}
		error = true;
		break;
	}
	if (t.type != token_type::unknown) {
		if (t.type == token_type::number)
			t.number = atof(t.text.c_str());
		tokens.push_back(t);
	}
	verify_tokenlist();
}

bool tokenizer::error_state() { return error;  }
token_list tokenizer::get_tokens() const { return tokens; }

class eval {
private:
	token_list tokens;
	bool error;
	double result;
	// returns operator precedence
	int precedence(const token& tk);
	bool is_number(const token& tk);
	bool is_operator(const token& tk);
	bool is_bracket(const token& tk);
	bool is_open_bracket(const token& tk);
	bool is_close_bracket(const token& tk);
	// returns true if bracket is found
	bool check_infix_for_brackets(const token_list& tl);
	// reorder tokens to postfix notation
	void to_postfix();
public:
	eval(const tokenizer& t);
	eval(const token_list& t);
	bool error_state();
	token_list get_tokens() const;
	double get_result();
	void solve();
};

eval::eval(const tokenizer& tk)
	: tokens{ tk.get_tokens() }, error{}, result{} {}

eval::eval(const token_list& t):
	tokens{ t }, error{}, result{} {}

token_list eval::get_tokens() const { return tokens; }
bool eval::error_state() { return error; }
double eval::get_result() { return result; }

int eval::precedence(const token& tk) {
	int precedence = -1;
	switch(tk.type) {
		case token_type::op_add:
		case token_type::op_sub:
			precedence = 10;
			break;
		case token_type::op_mul:
		case token_type::op_div:
			precedence = 20;
			break;
	}
	return precedence;
}

bool eval::is_number(const token& tk) {
	return (tk.type == token_type::number);
}

bool eval::is_operator(const token& tk) {
	if(tk.type == token_type::op_add) return true;
	if(tk.type == token_type::op_sub) return true;
	if(tk.type == token_type::op_mul) return true;
	if(tk.type == token_type::op_div) return true;
	return false;
}

bool eval::is_bracket(const token& tk) {
	if (is_open_bracket(tk)) return true;
	if (is_close_bracket(tk)) return true;
	return false;
}

bool eval::is_open_bracket(const token& tk) {
	return (tk.type == token_type::open_bracket);
}

bool eval::is_close_bracket(const token& tk) {
	return (tk.type == token_type::close_bracket);
}

/*  ~ The Shunting Yard Algorithm ~

       While there are tokens to be read:
       Read a token
       If it's a number add it to queue
       If it's an operator
              While there's an operator on the top of the stack with greater precedence:
                      Pop operators from the stack onto the output queue
              Push the current operator onto the stack
       If it's a left bracket push it onto the stack
       If it's a right bracket 
            While there's not a left bracket at the top of the stack:
				Pop operators from the stack onto the output queue.
             Pop the left bracket from the stack and discard it
	While there are operators on the stack, pop them to the queue */

void eval::to_postfix() {
	// input in infix notation
	token_list input { tokens };
	// output in postfix notation
	token_list output {};
	// stack of operators
	stack<token, token_list> op_stack{};

	// we reading token list from left to right
	for (token term : input) {
		// current token is the number
		if (is_number(term)) {
			output.push_back(term);
			continue;
		}
		// current token is the operator
		if (is_operator(term)) {
			// if the stack is empty
			if (op_stack.empty()) {
				op_stack.push(term);
				continue;
			}
			// if there are brackets on the top of the stack
			if (is_bracket(op_stack.top())) {
				op_stack.push(term);
				continue;
			}
			// checking operators priority
			if (precedence(term) > precedence(op_stack.top())) {
				op_stack.push(term);
			} 
			else {
				while (not op_stack.empty()) {
					output.push_back(op_stack.top());
					op_stack.pop();
				}
				op_stack.push(term);
			}
			continue;
		}
		// current token is the bracket
		if (is_bracket(term)) {
			if (is_open_bracket(term)) {
				op_stack.push(term);
				continue;
			}
			if (is_close_bracket(term)) {
				while (not op_stack.empty()) {
					if (is_open_bracket(op_stack.top())) {
						op_stack.pop();
						break;
					}
					if (not is_bracket(op_stack.top())) {
						output.push_back(op_stack.top());
					}
					op_stack.pop();
				}
				continue;
			}
		}
		// we must never get to this point
		error = true;
		break;
	}
	// finalization
	while (not op_stack.empty()) {
		output.push_back(op_stack.top());
		op_stack.pop();
	}
	tokens.assign(output.begin(), output.end());
	if (check_infix_for_brackets(tokens)) error = true;
}

bool eval::check_infix_for_brackets(const token_list& tl) {
	for (token term : tl)
		if (is_bracket(term)) return true;
	return false;
}

void eval::solve() {
	this->to_postfix();
	if (error) return;
	stack<token, token_list> nums{};
	for (token term : tokens) {
		if (is_number(term)) {
			nums.push(term);
			continue;
		}
		if (is_operator(term)) {
			double x{}, y{};
			token tr;
			tr.type = token_type::number;
			y = nums.top().number; nums.pop();
			x = nums.top().number; nums.pop();
			switch (term.type) {
				case token_type::op_add:
					tr.number = x + y;
					nums.push(tr);
					break;
				case token_type::op_sub:
					tr.number = x - y;
					nums.push(tr);
					break;
				case token_type::op_mul:
					tr.number = x * y;
					nums.push(tr);
					break;
				case token_type::op_div:
					tr.number = x / y;
					nums.push(tr);
					break;
			}
			continue;
		}
		// bad token list
		error = true;
		break;
	}
	this->result = nums.top().number;
}


void string_strip(string& str) {
	while (str.size() > 0 && isspace(str[0])) str.erase(str.begin());
	while (str.size() > 0 && isspace(str[str.size() - 1])) str.erase(str.end() - 1);
}

int main() {
	cout << "Simple math expression evaulator v " << VERSION << "\n"
		<<  "Unary + and - is not supported. Operations: + - * / \n"
		<<  "Use (.) for decimal point, blank line to exit \n\n";
	string line;
	do {
		cout << "(expr): ";
		getline(cin, line);
		string_strip(line);

		if (not line.empty()) {
			// we try to parse expression
			tokenizer tk(line);
			tk.parse();
			if (tk.error_state()) {
				cout << "-- parsing error --\n";
				continue;
			}
			// we try to evaulate expression
			eval ev(tk);
			ev.solve();
			if (ev.error_state()) {
				cout << "-- error --\n";
				continue;
			}
			else {
				cout << "(result): " << ev.get_result() << endl;
			}
			// debug
			//for (auto q : ev.get_tokens())
			//	cout << "{ " << (int)q.type << " \'" << q.text << "\' " << q.number << " }" << endl;
		}
	} while (not line.empty());
}