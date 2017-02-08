#include <stdio.h>
#include <stdlib.h>

#define arrayLength(array) (sizeof(array) / sizeof(array[0]))
#define fori(array) for(int i = 0; i < arrayLength(array); ++i)
#define forl(length) for(int i = 0; i < length; ++i)

inline void printErrorToConsole(char *errorMessage, int lineNumber) {
	printf("ERROR: %d:%s\n", lineNumber, errorMessage);
}

struct FileContent {
	size_t size;
	void *content;
};


bool writeFile(char *fileName, void *dataToWrite, int numOfBytes) {
	bool result = false;
	FILE *destFileHandle = fopen(fileName, "w+");
	if(fwrite(dataToWrite, 1, numOfBytes, destFileHandle) == numOfBytes) {
		result = true;
		//Successfully wrote the file
	} else {
		printErrorToConsole("Could not write file!", 0);
	}
	fclose(destFileHandle);
	return result;
}

FileContent readContentsOfFileWithNullTerminator(char *fileName) {
	FileContent res = {};
	FILE *srcFileHandle = fopen(fileName, "r+");
	if(srcFileHandle) {
		if(!fseek(srcFileHandle, 0, SEEK_END)) {
			int sizeOfSrcFile = ftell(srcFileHandle);
			if(!fseek(srcFileHandle, 0, SEEK_SET) && sizeOfSrcFile) {
				char *content = (char *)malloc(sizeOfSrcFile + 1);
				content[sizeOfSrcFile] = '\0';
				if(fread(content, 1, sizeOfSrcFile, srcFileHandle) == sizeOfSrcFile) {
					res.content = content;
					res.size = sizeOfSrcFile + 1;
				} else {
					printErrorToConsole("read file failed", __LINE__);	
					free(content);
				}
			} else {
				printErrorToConsole("couldn't seek to start of file", __LINE__);
			}
		} else {
			printErrorToConsole("couldn't seek to end of file", __LINE__);
		}
		fclose(srcFileHandle);
	} else {
		printErrorToConsole("File not found", __LINE__);
	}
	fclose(srcFileHandle);
	return res;
}

bool isNumeric(char charValue) {
	bool result = (charValue >= '0' && charValue <= '9');
	return result;
}

bool isAlphaNumeric(char charValue) {
	bool result = (charValue >= 65 && charValue <= 122);
	return result;
}

bool matchString(char *A, char *B) {
	bool res = true;
	while(*A && *B) {
		res = (*A++ == *B++);
		if(!res) break;
	} 
	return res;
}

bool matchString(char *A, char *B, int length) {
	char nullTerminateB[256];
	for(int i = 0; i < length; ++i) {
		nullTerminateB[i] = B[i];
	}
	nullTerminateB[length] = '\0';
	bool res = matchString(A, nullTerminateB);
	return res;
}


enum TokenType {
	TOKEN_UNINITIALISED,
	TOKEN_WORD,
	TOKEN_BOOl,
	TOKEN_STRING,
	TOKEN_INTEGER,
	TOKEN_FLOAT,
	TOKEN_SEMI_COLON,
	TOKEN_COLON,
	TOKEN_OPEN_BRACKET,
	TOKEN_CLOSE_BRACKET,
	TOKEN_NULL_TERMINATOR,
	TOKEN_OPEN_PARENTHESIS,
	TOKEN_NEWLINE,
	TOKEN_CLOSE_PARENTHESIS,
	TOKEN_FORWARD_SLASH,
	TOKEN_ASTRIX,
	TOKEN_COMMENT,
	TOKEN_HASH,
	TOKEN_PERIOD,
};

struct Token {
	TokenType type;
	char *at;
	int size;
};

struct Variable {
	char *name;
	char *value;
};

char * eatWhiteSpace(char *at) {
	while(*at == ' ' || *at == '\r' || *at == '\n') {
		at++;		
	}
	return at;
}
char * eatSpaces(char *at) {
	while(*at == ' ') {
		at++;		
	}
	return at;	
}

bool isNewLine(char value) {
	return (value == '\n' || value == '\r');
}

Token initToken(TokenType type, char *at, int size) {
	Token result = {type: TOKEN_UNINITIALISED, 0, 0};
	result.type = type;
	result.at = at;
	result.size = size;
	return result;
}

bool innerAlphaNumericCharacter(char value) {
	return (value == '-' || value == '_');
}

int stringLength(char *str) {
	int res = 0;
	while(*str) {
		str++;
		res++;
	}
	return res;
}

Token getToken(char *at, int *lineNumber, bool wantsToEatSpaces = true) {
	Token token = {TOKEN_UNINITIALISED, at, 1};
	if(wantsToEatSpaces) at = eatSpaces(at);
	switch(*at) {
		case ';': {
			token = initToken(TOKEN_SEMI_COLON, at, 1);
			at++;
		} break;
		case ':': {
			token = initToken(TOKEN_COLON, at, 1);
			at++;
		} break;
		case '.': {
			token = initToken(TOKEN_PERIOD, at, 1);
			at++;
		} break;
		case '\0': {
			token = initToken(TOKEN_NULL_TERMINATOR, at, 1);
			at++;
		} break;
		case '\r': 
		case '\n': {
			token = initToken(TOKEN_NEWLINE, at, 1);
			at++;
		} break;
		case '{': {
			token = initToken(TOKEN_OPEN_BRACKET, at, 1);
			at++;
		} break;
		case '}': {
			token = initToken(TOKEN_CLOSE_BRACKET, at, 1);
			at++;
		} break;
		case '(': {
			token = initToken(TOKEN_OPEN_PARENTHESIS, at, 1);
			at++;
		} break;
		case ')': {
			token = initToken(TOKEN_CLOSE_PARENTHESIS, at, 1);
			at++;
		} break;
		case '#': {
			token = initToken(TOKEN_HASH, at, 1);
			at++;
		} break;
		case '\'': 
		case '\"': {
			token = initToken(TOKEN_STRING, at, 1);
			char endOfString = (*at == '\"') ? '\"' : '\'';
			at++;
			while(*at && *at != endOfString) {
				if(isNewLine(*at)) {
					*lineNumber = *lineNumber + 1;
				}
				at++;
			}
			if(*at == endOfString) at++;
			token.size = (at - token.at);//quotation are kept with the value
		} break;
		case '/': {
			token = initToken(TOKEN_FORWARD_SLASH, at, 1);
			if(matchString(at, "//")) {
				token.type = TOKEN_COMMENT;
				at += 2;
				while(*at && !isNewLine(*at)) {
					at++;
				}
			} else if(matchString(at, "/*")) {
				token.type = TOKEN_COMMENT;
				at += 2;

				while(*at && !matchString(at, "*/")) {
					if(isNewLine(*at)) {
						*lineNumber = *lineNumber + 1;
					}
					at++;
				}
				if(*at) at += 2;
			}
			token.size = at - token.at;
		} break;
		default: {
			token.at = at;
			if(isAlphaNumeric(*at)) {
				token = initToken(TOKEN_WORD, at, 1);
				while(*at && (isAlphaNumeric(*at) || isNumeric(*at) || innerAlphaNumericCharacter(*at))) {
					at++;
				}
				token.size = at - token.at;
				if(matchString(token.at, "true") && token.size == stringLength("true")) token.type = TOKEN_BOOl;
				else if(matchString(token.at, "false") && token.size == stringLength("false")) token.type = TOKEN_BOOl;

			} else if(isNumeric(*at)) {
				token = initToken(TOKEN_INTEGER, at, 1);
				bool numberOfDecimal = 0;
				while(*at && (isNumeric(*at) || *at == '.')) {
					if(*at == '.') {
						numberOfDecimal++;
						if(numberOfDecimal > 1) {
							printErrorToConsole("found more than one colon in number", *lineNumber);
							break;
						}
					}
					at++;
				}
				if(numberOfDecimal > 0) token.type = TOKEN_FLOAT;
				token.size = at - token.at;
			}
			
			
		}
	}
	return token;
}

Variable *getVariable(char *name, int length, Variable *vars, int count) {
	Variable *result = 0;
	for(int i = 0; i < count; ++i) {
		Variable *var = vars + i;
		if(matchString(var->name, name, length)) {
			result = var;
			break;
		}
	}
	return result;
}



char *copyString(char *stringToCopy, int length) {
	char *res = (char *)malloc(length + 1);
	for(int i = 0; i < length; ++i) {
		res[i] = stringToCopy[i];
	}
	res[length] = '\0';
	return res;
}

void advancePtrWithToken(char **ptr, Token token) {
	*ptr = (token.at + token.size);
}

int main(int argCount, char *args[]) {
	if(argCount > 1) {
		char *fileName = args[1];
		FileContent content = readContentsOfFileWithNullTerminator(fileName);
		if(content.size != 0) {
			
			//TODO: change this to dynamically allocate memory if needed, right now we just take a big chunk and hope we fit!!!!!
			int sizeOfDestBuffer = 2*content.size;
			char *destBuffer = (char *)malloc(sizeOfDestBuffer); 
			destBuffer[content.size] = '\0';
			char *destAt = (char *)destBuffer;
			Variable varArray[256];
			int varArrayCount = 0;
			bool parsing = true;
			int lineNumber = 1;
			char *src = (char *)content.content;
			char *lastSrc = (char *)content.content;
			while(parsing) {
				bool copySrc = true;
				Token token = getToken(src, &lineNumber);
				advancePtrWithToken(&src, token);
				switch(token.type) {
					case TOKEN_UNINITIALISED: {
					} break;
					case TOKEN_SEMI_COLON: {

					} break;
					case TOKEN_COLON: {

					} break;
					case TOKEN_OPEN_BRACKET: {

					} break;
					case TOKEN_CLOSE_BRACKET: {

					} break;
					case TOKEN_NULL_TERMINATOR: {
						parsing = false;
					} break;
					case TOKEN_PERIOD: {
					} break;
					case TOKEN_OPEN_PARENTHESIS: {

					} break;
					case TOKEN_STRING: {

					} break;
					case TOKEN_NEWLINE: {
						lineNumber++;
					} break;
					case TOKEN_CLOSE_PARENTHESIS: {

					} break;
					case TOKEN_FORWARD_SLASH: {

					} break;
					case TOKEN_ASTRIX: {

					} break;
					case TOKEN_COMMENT: {
					} break;
					case TOKEN_INTEGER: {
					} break;
					case TOKEN_HASH: {
						Token nextToken = getToken(src, &lineNumber, false);
						if(nextToken.type == TOKEN_WORD && matchString(nextToken.at, "define")) {
							copySrc = false;
							advancePtrWithToken(&src, nextToken);
							Token wordToken = getToken(src, &lineNumber);
							if(wordToken.type == TOKEN_WORD) {
								advancePtrWithToken(&src, wordToken);
								Token valueToken = getToken(src, &lineNumber);
								if(valueToken.type == TOKEN_INTEGER || valueToken.type == TOKEN_WORD || valueToken.type == TOKEN_STRING || valueToken.type == TOKEN_BOOl) {
									advancePtrWithToken(&src, valueToken);
									if(varArrayCount < arrayLength(varArray)) {
										Variable *refVar = getVariable(valueToken.at, valueToken.size, varArray, varArrayCount);
										Variable *newVar = varArray + varArrayCount++;
										if(refVar) {
											newVar->name = copyString(wordToken.at, wordToken.size);
											newVar->value = refVar->value;	
										} else {
											newVar->name = copyString(wordToken.at, wordToken.size);
											newVar->value = copyString(valueToken.at, valueToken.size);	
										}
									} else {
										printErrorToConsole("variable array full", lineNumber);	
										parsing = false;		
									}
								} else if(wordToken.type != TOKEN_NEWLINE && wordToken.type != TOKEN_NULL_TERMINATOR) {
									printErrorToConsole("expected a value or a variable", lineNumber);	
									parsing = false;
								}
							} else if(wordToken.type != TOKEN_NEWLINE && wordToken.type != TOKEN_NULL_TERMINATOR) {
								printErrorToConsole("expected a word literal instead got a ", lineNumber);
								parsing = false;
							}
						}
					} break;
					case TOKEN_WORD: {
						printf("%s\n", copyString(token.at, token.size));
						Variable *refVar = getVariable(token.at, token.size, varArray, varArrayCount);
						if(refVar) {

							copySrc = false;
							//copy in any spaces
							while(lastSrc < token.at) {
								*destAt++ = *lastSrc++;	

							}

							for(int i = 0; i < stringLength(refVar->value); ++i) {
								*destAt++ = refVar->value[i];	

							}
						}
					} break;
					default: {
						printf("Token type %d not supported\n", token.type);
					}
				}

				if(copySrc) {
					while(lastSrc < src) {
						*destAt++ = *lastSrc++;
					}	
				}
				lastSrc = src;
			}
			forl(varArrayCount) {
				printf("variable[%d]:\nname:%s\nvalue:%s\n", i, varArray[i].name, varArray[i].value);
			}
			int bytesToWrite = (int)(destAt - destBuffer);
			writeFile("style.css", destBuffer, bytesToWrite - 1);
		}
	}

	return 0;
}