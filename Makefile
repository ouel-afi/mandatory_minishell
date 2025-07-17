SRCS = main.c

CC = cc
CFLAGS = -I/home/linuxbrew/.linuxbrew/opt/readline/include  # -Wall -Wextra -Werror
LDFLAGS = -L/home/linuxbrew/.linuxbrew/opt/readline/lib -lreadline

NAME = minishell
OBJS = $(SRCS:.c=.o)

LIBFT_DIR = ./libft
LIBFT = $(LIBFT_DIR)/libft.a

all = $(NAME)

$(NAME): $(OBJS) $(LIBFT)
	$(CC) $(CFLAGS) $(OBJS) $(LIBFT) $(LDFLAGS) -o $(NAME)

$(LIBFT):
	$(MAKE) -C $(LIBFT_DIR)
	
%.o: %.C
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
	$(MAKE) clean -C $(LIBFT_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all