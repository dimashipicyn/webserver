CC = clang++
FLAGS = -Wall -Wextra -MMD -std=c++98 -Wshadow #-Wno-shadow # -fsanitize=address
SRCS = $(shell find . -type f -name "*.cpp" -exec basename {} *.cpp \;)
VPATH = autoindex:cgi:utils:http:settingsManager:event
INCLUDES = -Ihttp -Iutils -Icgi -IsettingsManager -Ievent -Iautoindex
OBJ = $(SRCS:.cpp=.o)
DEPENDS = $(SRCS:.cpp=.d)
NAME = webserv

.PHONY: all clean fclean re run

all: $(SRCS) $(NAME)

$(NAME): $(OBJ)
		$(CC) $(FLAGS) $^ -o $@

.cpp.o: $(SRC)
		$(CC) $(FLAGS) -c $< $(INCLUDES)

clean:
		@rm -rf $(OBJ) $(DEPENDS)

fclean: clean
		@rm -rf $(NAME) $(LFT)

re: fclean all

-include ${DEPENDS}
