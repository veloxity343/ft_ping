# Standard
NAME	=	libft.a

# Directories
INC		=	inc/
SRC_DIR	=	src
OBJ_DIR	=	obj

# Compiler & flags
WFLAGS	=	-Wall -Wextra -Werror
IFLAGS	=	-I$(INC)
CC		=	gcc
RM		=	rm -rf
AR		=	ar rcs

# Sources & objects
SRC		=	$(shell find $(SRC_DIR) -type f -name "*.c")
OBJ		=	$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))

# Colors
RED		=	\033[1;31m
GREEN	=	\033[1;32m
YELLOW	=	\033[1;33m
BLUE	=	\033[1;34m
RESET	=	\033[0m

# Build Rules
all:	$(NAME)
	@echo "$(GREEN)Successfully compiled libft!$(RESET)"

$(NAME):	$(OBJ)
	@echo "\n"
	@echo "$(BLUE)Creating$(RESET)\t$(NAME)"
	@$(AR) $(NAME) $(OBJ)
	@echo "$(GREEN)Successfully created libft archive!$(RESET)"

$(OBJ_DIR)/%.o:	$(SRC_DIR)/%.c
	@mkdir -p $(@D)
	@printf "$(YELLOW)Compiling\t$(RESET)libft %-33.33s\r" $@
	@$(CC) $(WFLAGS) $(IFLAGS) -c $< -o $@

# Scruba dub dub
clean:
	@if [ -d "$(OBJ_DIR)" ]; then \
	$(RM) -rf $(OBJ_DIR); \
	echo "$(RED)Deleting$(RESET)\tlibft "$(OBJ_DIR); else \
	echo "$(BLUE)No libft objects to remove.$(RESET)"; \
	fi;

fclean:	clean
	@if [ -f "$(NAME)" ]; then \
	$(RM) -f $(NAME); \
	echo "$(RED)Deleting$(RESET)\t"$(NAME); else \
	echo "$(BLUE)No $(NAME) to remove.$(RESET)"; \
	fi;

re:	fclean all

.PHONY:	all clean fclean re
