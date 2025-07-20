# SRCS = main.c

# CC = cc
# CFLAGS = -I/home/linuxbrew/.linuxbrew/opt/readline/include  # -Wall -Wextra -Werror
# LDFLAGS = -L/home/linuxbrew/.linuxbrew/opt/readline/lib -lreadline

# NAME = minishell
# OBJS = $(SRCS:.c=.o)

# LIBFT_DIR = ./libft
# LIBFT = $(LIBFT_DIR)/libft.a

# all = $(NAME)

# $(NAME): $(OBJS) $(LIBFT)
# 	$(CC) $(CFLAGS) $(OBJS) $(LIBFT) $(LDFLAGS) -o $(NAME)

# $(LIBFT):
# 	$(MAKE) -C $(LIBFT_DIR)
	
# %.o: %.C
# 	$(CC) $(CFLAGS) -c $< -o $@

# clean:
# 	rm -f $(OBJS)
# 	$(MAKE) clean -C $(LIBFT_DIR)

# fclean: clean
# 	rm -f $(NAME)

# re: fclean all

SRCS =  main.c
CC = cc
# CFLAGS = -Wall -Wextra -Werror
NAME = minishell

OBJS = $(SRCS:.c=.o)

LIBFT_DIR = ./libft
LIBFT = $(LIBFT_DIR)/libft.a

all: $(NAME)

$(NAME): $(OBJS) $(LIBFT)
	$(CC) $(CFLAGS) -L/Users/ouel-afi/goinfre/homebrew/opt/readline/lib -lreadline $(OBJS) $(LIBFT) -o $(NAME)

$(LIBFT):
	$(MAKE) -C $(LIBFT_DIR)

%.o: %.c 
	$(CC) $(CFLAGS) -I/Users/ouel-afi/goinfre/homebrew/opt/readline/include -c $< -o $@

clean:
	rm -f $(OBJS) 
	$(MAKE) clean -C $(LIBFT_DIR)

fclean: clean
	rm -f $(NAME)
	$(MAKE) fclean -C $(LIBFT_DIR)

re: fclean all

.PHONY: all bonus clean fclean re