# Standard
NAME		=	ft_ping

# Directories
LIBFT		=	./libft/libft.a
INC			=	inc/
LIBFT_INC	=	libft/inc/
SRC_DIR		=	src
OBJ_DIR		=	obj

# Compiler & flags
WFLAGS		=	-Wall -Wextra -Werror #-g3 -fsanitize=address
IFLAGS		=	-I$(INC) -I$(LIBFT_INC)
DSYM		=	-g3
FSAN		=	-fsanitize=address $(DSYM)
CFLAGS		=	$(WFLAGS) $(IFLAGS)
CC			=	gcc
AR			=	ar -rcs
RM			=	rm -rf

# Sources & objects
SRC			=	$(shell find $(SRC_DIR) -type f -name "*.c")
OBJ			=	$(patsubst $(SRC_DIR)%.c, $(OBJ_DIR)%.o, $(SRC))

# Colors
RED		=	\033[1;31m
GREEN	=	\033[1;32m
YELLOW	=	\033[1;33m
BLUE	=	\033[1;34m
RESET	=	\033[0m

# Build rules
all:		$(LIBFT) $(NAME)
	@echo "$(GREEN)Successfully compiled!$(RESET)"

$(LIBFT):
	@echo "$(BLUE)Building$(RESET)\tlibft"
	@make -C $(dir $(LIBFT))

$(NAME):	$(OBJ)
	@echo "\n$(BLUE)Building$(RESET)\t$(NAME)"
	@make -sC $(dir $(LIBFT))
	@$(CC) $(CFLAGS) -o $(NAME) $(OBJ) $(LIBFT)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@printf "$(YELLOW)Compiling\t$(RESET)%-33.33s\r" $@
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@if [ -d "$(OBJ_DIR)" ]; then \
	$(RM) -rf $(OBJ_DIR); \
	echo "$(RED)Deleting$(RESET)\t"$(OBJ_DIR); else \
	echo "$(BLUE)No $(NAME) objects to remove.$(RESET)"; \
	fi;

fclean:	clean
	@if [ -f "$(NAME)" ]; then \
	$(RM) -f $(NAME); \
	echo "$(RED)Deleting$(RESET)\t"$(NAME); else \
	echo "$(BLUE)No executable $(NAME) to remove.$(RESET)"; \
	fi;
	@make fclean -C $(dir $(LIBFT))

re:		fclean all

asan:		CFLAGS	+=	$(FSAN)
asan:	re
	@echo "$(YELLOW)Running with AddressSanitizer...$(RESET)"
	sudo ./$(NAME) 8.8.8.8

valgrind:	$(NAME)
	@echo "$(YELLOW)Running with Valgrind...$(RESET)"
	sudo valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(NAME) 8.8.8.8

norm:
	norminette $(SRC_DIR) $(INC)
	@echo  "$(GREEN)Norminette check complete!$(RESET)"

.PHONY:	all clean fclean re asan valgrind norm
