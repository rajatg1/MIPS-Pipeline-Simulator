#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>

using namespace std;

string output_registers;
string input_registers;
vector<string> prog_lines;
int prog_registers[8];
int in_stage_registers[8];
bool branch_flag = false;
int cycles_count = 0;
int clocky;
vector<string> original_prog_lines;
vector<int> final_register_values;
void printOriginalProgramLines(int);
void printProgramRegistors();
void printInStageRegisters();
void putIntoRegisters(string);
void readInput(ifstream &file_in);
void putLabelsInProgLines();
int checkLabel(string);
int extractIntFromWordVector(string);
int checkForCondition(string, bool&);
void executeAdd(vector<string> &word_vector);
void executeAddI(vector<string> &word_vector);
void executeSub(vector<string> &word_vector);
void executeMul(vector<string> &word_vector);
void executeDiv(vector<string> &word_vector);
int executeWordVector(int, vector<string> &word_vector);
int executeInstruction(string, bool&);
int resolveBranch(vector<string> &word_vector, int);
void instructionFetch(int &, int);
void instructionExecute(int &, int);
void instructionWriteBack(int &, int);

void printOriginalProgramLines(int i){
		cout << original_prog_lines[i] << endl;
}

void printProgramRegistors(){
	for(int i=0; i<8; i++){
		cout << prog_registers[i];
		if(i!=7)
			cout << ",";
	}
	cout << endl << endl;
}

void copyRegisterValuesIntoInstage(){
	for(int i=0; i<8; i++){
		in_stage_registers[i] = prog_registers[i];
	}
}

void printInStageRegisters(){
	for(int i=0; i<8; i++){
			cout << in_stage_registers[i];
			if(i!=7)
				cout << ",";
		}
		cout << endl << endl;
}

void putIntoRegisters(string line){
		stringstream ss(line);
	    int i = -1;
	    int val = 0;
	    while (ss >> val)
	    {
	    		i++;
	        prog_registers[i] = val;
	        if (ss.peek() == ',')
	            ss.ignore();
	    }
}

void putLabelsInProgLines(){
	for(int i=0; i < prog_lines.size(); i++){
		int k = prog_lines[i].find("label");
		if(k == -1 || k > 1){
			prog_lines[i] = "labelx " + prog_lines[i];
		}
	}
}

void readInput(ifstream &file_in){
	int iteration_count = 0;
	string line = "";
	cout << endl;
		while(getline(file_in, line)){
			iteration_count++;
			if(iteration_count == 1){
				input_registers = line;
				putIntoRegisters(line);
			}
			else{
				prog_lines.push_back(line);
				original_prog_lines.push_back(line);
			}
			line = "";
		}
	putLabelsInProgLines();
}

int checkForCondition(string word, bool &branch_flag1){
	int val = -1;
	branch_flag1 = false;
	if(word.compare("add") == 0)
		val = 1;
	else if(word.compare("addi") == 0)
		val = 2;
	else if(word.compare("sub") == 0)
		val = 3;
	else if(word.compare("mul") == 0)
		val = 4;
	else if(word.compare("div") == 0)
		val = 5;
	else if(word.compare("b") == 0){
		val = 6;
		branch_flag1 = true;
	}
	else if(word.compare("beq") == 0){
		val = 7;
		branch_flag1 = true;
	}
	else if(word.compare("bnq") == 0){
		val = 8;
		branch_flag1 = true;
	}
	else if(word.compare("end") == 0)
		val = 9;
	return val;
}

int extractIntFromWordVector(string line){
	int k = line.find('$');
	string num = line.substr(k+1);
	int val = 0;
	stringstream ss(num);
	ss >> val;
	return val;
}

void executeAdd(vector<string> &word_vector){
	int d = extractIntFromWordVector(word_vector[0]);
	int s1 = extractIntFromWordVector(word_vector[1]);
	int s2 = extractIntFromWordVector(word_vector[2]);
	prog_registers[d] = prog_registers[s1] + prog_registers[s2];
}

void executeAddI(vector<string> &word_vector){
	int d = extractIntFromWordVector(word_vector[0]);
	int s1 = extractIntFromWordVector(word_vector[1]);
	int val = extractIntFromWordVector(word_vector[2]);
	prog_registers[d] = prog_registers[s1] + val;
}

void executeSub(vector<string> &word_vector){
	int d = extractIntFromWordVector(word_vector[0]);
	int s1 = extractIntFromWordVector(word_vector[1]);
	int s2 = extractIntFromWordVector(word_vector[2]);
	prog_registers[d] = prog_registers[s1] - prog_registers[s2];
}

void executeMul(vector<string> &word_vector){
	int d = extractIntFromWordVector(word_vector[0]);
	int s1 = extractIntFromWordVector(word_vector[1]);
	int s2 = extractIntFromWordVector(word_vector[2]);
	prog_registers[d] = prog_registers[s1] * prog_registers[s2];
}

void executeDiv(vector<string> &word_vector){
	int d = extractIntFromWordVector(word_vector[0]);
	int s1 = extractIntFromWordVector(word_vector[1]);
	int s2 = extractIntFromWordVector(word_vector[2]);
	prog_registers[d] = prog_registers[s1] / prog_registers[s2];
}

int executeWordVector(int k, vector<string> &word_vector){
	if(k == 1){
		executeAdd(word_vector);
		branch_flag = false;
		return 0;
	}
	else if(k == 2){
		executeAddI(word_vector);
		branch_flag = false;
		return 0;
	}
	else if(k == 3){
		executeSub(word_vector);
		branch_flag = false;
		return 0;
	}
	else if(k == 4){
		executeMul(word_vector);
		branch_flag = false;
		return 0;
	}
	else if(k == 5){
		executeDiv(word_vector);
		branch_flag = false;
		return 0;
	}
	else if(k == 6 || k == 7 || k == 8){
		branch_flag = true;
		return resolveBranch(word_vector, k);
	}
	else if(k == 9){
		branch_flag = false;
		return -1;
	}
	return k;
}

int executeInstruction(string line, bool &branch_flag){
	vector<string> word_vector;
	stringstream ss(line);
	string word;
	int k = 0;
	bool branch_flag1 = false;
	getline(ss, word, ' ');
	getline(ss, word, ' ');
	k = checkForCondition(word, branch_flag1);
	branch_flag = branch_flag1;
	while(getline(ss, word, ',')){
		word_vector.push_back(word);
	}
	int res = executeWordVector(k, word_vector);
	return res;
}

int checkLabel(string label){
	for(int i=0; i < prog_lines.size(); i++){
		stringstream ss(prog_lines[i]);
		string word;
		getline(ss, word, ' ');
		if(label.compare(word) == 0)
			return i;
	}
	return -1;
}

int resolveBranch(vector<string> &word_vector, int k){
	int final_val = -2;
	int s1 = 0;
	int s2 = 0;
	if(k == 6){
		final_val = checkLabel(word_vector[0]);
	}
	else if(k == 7){
		s1 = extractIntFromWordVector(word_vector[0]);
		s2 = extractIntFromWordVector(word_vector[1]);
		if(s1 == s2){
			final_val = checkLabel(word_vector[2]);
		}
	}
	else if(k == 8){
		s1 = extractIntFromWordVector(word_vector[0]);
		s2 = extractIntFromWordVector(word_vector[1]);
		if(s1 != s2){
			final_val = checkLabel(word_vector[2]);
		}
	}
	return final_val;
}

void instructionFetch(int &cycle, int k){
	chrono::milliseconds timespan1(clocky);
	this_thread::sleep_for(timespan1);
	cycle++;
	cout << cycle << " - Fetched instruction " << endl;
	printOriginalProgramLines(k);
	printInStageRegisters();
	this_thread::sleep_for(timespan1);
}

void instructionExecute(int &cycle, int k){
	chrono::milliseconds timespan1(clocky);
	this_thread::sleep_for(timespan1);
	cycle++;
	cout << cycle << " - Executed instruction " <<  endl;
	printOriginalProgramLines(k);
	printInStageRegisters();
	this_thread::sleep_for(timespan1);
}


void instructionWriteBack(int &cycle, int k){
	chrono::milliseconds timespan1(clocky);
	this_thread::sleep_for(timespan1);
	cycle++;
	cout << cycle << " - WriteBacked instruction " <<  endl;
	printOriginalProgramLines(k);
	printProgramRegistors();
	this_thread::sleep_for(timespan1);
}

int performInstruction(){
	cout << endl;
	int clock_cycle = 0;
	int result = 0;
	bool branch_flag = false;
	bool prev_branch_flag = false;
	for(int i=0; i < prog_lines.size(); i++){
		copyRegisterValuesIntoInstage();
		result = executeInstruction(prog_lines[i], branch_flag);
		if(result == -1){
			printOriginalProgramLines(i);
			cout << endl;
			return -1;
			break;
		}
		else{
			if(result == 0){
				if(clock_cycle == 0 || prev_branch_flag == true){
					instructionFetch(clock_cycle, i);
					instructionExecute(clock_cycle, i);
					instructionWriteBack(clock_cycle, i);
					prev_branch_flag = false;
				}
				else if(branch_flag == false){
					instructionWriteBack(clock_cycle, i);
				}
			}
			else{
				if(result == -2){
					prev_branch_flag = true;
				}
				else{
					i = result;
					i--;
					prev_branch_flag = true;
				}

			}
		}
	}
	return 0;
}

class solution {

private:

bool DEBUG;
int clck;
vector<string> vect_lines;
vector<int>* t_vars;

public :

solution(ifstream &file_in,int clck_in = 10 ,bool DEBUG_in = true){
	this->clck = clck_in;
	this->DEBUG = true;
	readInput(file_in);
	this->t_vars = new vector<int>;
	clocky = clck_in;
}

//void dbg(const string &msg);

vector<int>* alu();

int mips_clock();

};



int solution::mips_clock() {
chrono::milliseconds timespan(clck);

this_thread::sleep_for(timespan);
static int cycle = 0;
if (cycle == 0 )
	cycle = 1;
else
	cycle = 0;
return cycle;
}

vector<int>* solution::alu(){

	int ending_condition = 0;
	while(true){
		if(ending_condition == -1){
			break;
		}

		int clock = this->mips_clock();
		if(!clock){
			ending_condition = performInstruction();
		}
		else{
			continue;
		}
	}

	for(int i=0; i<8; i++){
		this->t_vars->push_back(prog_registers[i]);
	}

	return this->t_vars;
}

