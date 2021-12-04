CC = clang++
FLAGS = -Wall -Wextra -MMD -std=c++98 -Wshadow #-Wno-shadow # -fsanitize=address
SRCS = $(shell find . -type f -name "*.cpp")

OBJ = $(SRCS:.cpp=.o)
BUILDS = builds/
DEPENDS = ${SRCS:.cpp=.d}
NAME = a.out

.PHONY: all clean fclean re run

all: $(SRCS) $(NAME) run

run:
	./$(NAME)

$(NAME): $(OBJ)
		$(CC) $(FLAGS) $(OBJ) -o $(NAME)

.cpp.o: $(SRCS)
		$(CC) $(FLAGS) -c $<

clean:
		@rm -rf $(OBJ) $(DEPENDS)

fclean: clean
		@rm -rf $(NAME) $(LFT)

re: fclean all

-include ${DEPENDS}
