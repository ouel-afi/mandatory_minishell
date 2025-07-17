/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ouel-afi <ouel-afi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/09 15:45:13 by ouel-afi          #+#    #+#             */
/*   Updated: 2025/07/15 12:25:59 by ouel-afi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "minishell.h"

int g_heredoc_interrupted = 0;

typedef struct s_lexer {
	int position;
	int length;
	char *input;
}	t_lexer;

typedef enum s_type {
    CMD = 1,
	PIPE,	
	SINGLE_QUOTE,
	DOUBLE_QUOTE,
    REDIR_IN,
    REDIR_OUT,
    APPEND,
    HEREDOC
} t_type;

typedef struct s_token {
	char	*value;
	t_type		type;
	int			has_space;
	int expand_heredoc;
	char **cmds;
	struct s_token *redir;
	int fd;
	struct s_token *next;
}	t_token;

typedef struct s_env
{
	char *name;
	char *value;
	char **env;
	struct s_env *next;
}	t_env;

t_token	*create_token(char *value, char quote, int has_space)
{
	t_token	*token;

	token = malloc(sizeof(t_token));
	if (!token)
		return (NULL);
	else
	{
		token->value = ft_strdup(value);
		token->next = NULL;
	}
	// if (!token->value){
	// 	free(token);
	// 	return NULL;
	// }
	if (quote == '\'')
		token->type = 3;
	else if (quote == '"')
		token->type = 4;
	else
		token->type = 0;
	token->has_space = has_space;
	token->expand_heredoc = 0;
	token->cmds = NULL;
	token->redir = NULL;
	token->fd = -1;
	return (token);
}

int	is_special_char(char c)
{
	return (ft_strchr("|<>()&", c) != NULL);
}

int	is_quote(char c)
{
	return (c == '\'' || c == '"');
}

void	handle_word_quote(char **result, t_lexer *lexer, int *in_quotes)
{
	char	quote;
	char	*temp;
	char	*new_result;
	size_t	start;

	quote = lexer->input[lexer->position++];
	start = lexer->position;
	*in_quotes = 1;
	while (lexer->position < lexer->length
		&& lexer->input[lexer->position] != quote)
		lexer->position++;
	if (lexer->position >= lexer->length)
	{
		ft_putstr_fd("bash : syntax error unclosed quotes\n", 2);
		return ;
	}
	temp = ft_substr(lexer->input, start, lexer->position - start);
	new_result = ft_strjoin(*result, temp);
	free(*result);
	*result = new_result;
	lexer->position++;
	*in_quotes = 0;
	free(temp);
}

int	is_space(t_lexer *lexer)
{
	if ((lexer->input[lexer->position] >= 9
			&& lexer->input[lexer->position] <= 13)
		|| lexer->input[lexer->position] == 32)
		return (1);
	return (0);
}

void	handle_plain_word(char **result, t_lexer *lexer)
{
	size_t	start;
	char	*temp;
	char	*new_result;

	start = lexer->position;
	while (lexer->position < lexer->length && !ft_strchr("\"'|<>()& ",
			lexer->input[lexer->position]))
		lexer->position++;
	temp = ft_substr(lexer->input, start, lexer->position - start);
	new_result = ft_strjoin(*result, temp);
	free(temp);
	free(*result);
	*result = new_result;
}

t_token	*handle_word(t_lexer *lexer)
{
	char	*result;
	int		has_space;
	int		in_quotes;
	t_token	*token;

	result = ft_strdup("");
	if (!result)
		return (NULL);
	in_quotes = 0;
	while (lexer->position < lexer->length && !is_space(lexer))
	{
		if (is_quote(lexer->input[lexer->position]))
			handle_word_quote(&result, lexer, &in_quotes);
		else if (!in_quotes && is_special_char(lexer->input[lexer->position]))
			break ;
		else
			handle_plain_word(&result, lexer);
	}
	if (lexer->position < lexer->length && lexer->input[lexer->position] == ' ')
		has_space = 1;
	else
		has_space = 0;
	token = create_token(result, 0, has_space);
	free(result);
	return (token);
}

t_type	token_type(t_token *token)
{
	if (token->type == 3)
		return (SINGLE_QUOTE);
	else if (token->type == 4)
		return (DOUBLE_QUOTE);
	else if (strcmp(token->value, "|") == 0)
		return (PIPE);
	else if (strcmp(token->value, ">>") == 0)
		return (APPEND);
	else if (strcmp(token->value, "<<") == 0)
		return (HEREDOC);
	else if (strcmp(token->value, "<") == 0)
		return (REDIR_IN);
	else if (strcmp(token->value, ">") == 0)
		return (REDIR_OUT);
	else
		return (CMD);
}

void	append_token(t_token **head, t_token *token)
{
	t_token	*tmp;

	tmp = NULL;
	if (!token)
		return ;
	if (!*head)
	{
		*head = token;
		return ;
	}
	tmp = *head;
	while (tmp->next)
		tmp = tmp->next;
	tmp->next = token;
}

t_token	*handle_quote(t_lexer *lexer, char quote)
{
	t_token *token = NULL;
	size_t	length;
	size_t	start;
	char	*value;

	lexer->position += 1;
	start = lexer->position;
	while (lexer->position < lexer->length
		&& lexer->input[lexer->position] != quote)
		lexer->position++;
	if (lexer->position >= lexer->length)
	{
		ft_putstr_fd("bash : syntax error unclosed quote\n", 2);
		return (0);
	}
	length = lexer->position - start;
	value = ft_substr(lexer->input, start, length);
	lexer->position += 1;
	if (lexer->input[lexer->position] == 32)
		token = create_token(value, quote, 1);
	else
		token = create_token(value, quote, 0);
	free(value);
	return (token);
}

t_token	*handle_operations(t_lexer *lexer, char *oper, int i)
{
	char	*str;
	t_token *token;

	str = ft_substr(oper, 0, i);
	if (!str)
		return (NULL);
	lexer->position += i;
	if (lexer->input[lexer->position] == 32)
		token = create_token(str, str[0], 1);
	else
		token = create_token(str, str[0], 0);
	free(str); 
	return (token);
}

t_lexer	*initialize_lexer(char *input)
{
	t_lexer	*lexer;

	lexer = malloc((sizeof(t_lexer)));
	if (!lexer)
		return (NULL);
	lexer->input = input;
	lexer->length = ft_strlen(input);
	lexer->position = 0;
	return (lexer);
}

void	skip_whitespace(t_lexer *lexer)
{
	while (lexer->position < lexer->length && is_space(lexer))
		lexer->position++;
}

t_token	*get_next_token(t_lexer *lexer)
{
	char	*current;

	skip_whitespace(lexer);
	if (lexer->position >= lexer->length)
		return (NULL);
	current = lexer->input + lexer->position;
	if (current[0] == '\'' || current[0] == '"')
		return (handle_quote(lexer, *current));
	if ((lexer->input[lexer->position] == '|'
			&& lexer->input[lexer->position + 1] == '|')
		|| (lexer->input[lexer->position] == '&'
			&& lexer->input[lexer->position + 1] == '&'))
		return (handle_operations(lexer, current, 2));
	if ((lexer->input[lexer->position] == '>'
			&& lexer->input[lexer->position + 1] == '>')
		|| (lexer->input[lexer->position] == '<'
			&& lexer->input[lexer->position + 1] == '<'))
		return (handle_operations(lexer, current, 2));
	if (current[0] == '|' || current[0] == '<' || current[0] == '>'
		|| current[0] == '(' || current[0] == ')' || current[0] == '&')
		return (handle_operations(lexer, current, 1));
	return (handle_word(lexer));
}

void free_lexer(t_lexer *lexer)
{
	if (!lexer)
		return;
	free(lexer);
}

void free_token(t_token *token)
{
    int i = 0;

    if (!token)
        return;

    if (token->value)
        free(token->value);

    if (token->cmds)
    {
        while (token->cmds[i])
        {
            free(token->cmds[i]);
            i++;
        }
        free(token->cmds);
    }
    free(token);
}

void free_token_list(t_token *token)
{
    t_token *current;
    t_token *next;
    t_token *redir_tmp;
    t_token *redir_next;
    int i;

    current = token;
    while (current)
    {
        next = current->next;
        if (current->value)
            free(current->value);

        if (current->cmds)
        {
            i = 0;
            while (current->cmds[i])
            {
                free(current->cmds[i]);
                i++;
            }
            free(current->cmds);
        }
        redir_tmp = current->redir;
        while (redir_tmp)
        {
            redir_next = redir_tmp->next;
            if (redir_tmp->value)
                free(redir_tmp->value);

            if (redir_tmp->cmds)
            {
                i = 0;
                while (redir_tmp->cmds[i])
                {
                    free(redir_tmp->cmds[i]);
                    i++;
                }
                free(redir_tmp->cmds);
            }

            free(redir_tmp);
            redir_tmp = redir_next;
        }

        free(current);
        current = next;
    }
}

int check_errors(t_token *token)
{
	t_token *tmp;
	if (!token)
	return (1);
	if (token->type == 2)
	{
		printf("bash: syntax error near unexpected token `|'\n");
		return (1);
	}
	tmp = token;
	while (tmp)
	{
		if ((tmp->type == 2 || tmp->type == 5 || tmp->type == 6 || tmp->type == 7 || tmp->type == 8) && !tmp->next)
		{
			printf("bash: syntax error near unexpected token `newline'\n");
			return (1);
		}
		if ((tmp->type == 5 || tmp->type == 6 || tmp->type == 7 || tmp->type == 8) && tmp->next && (tmp->next->type == 2 || tmp->next->type == 5 || tmp->next->type == 6 || tmp->next->type == 7 || tmp->next->type == 8))
		{
			printf("bash: syntax error near unexpected token `%s'\n", tmp->next->value);
			return (1);
		}
		tmp = tmp->next;
	}
	return (0);
}

int	is_alphanumeric(int c)
{
	return ((c >= 'A' && c <= 'Z')
		|| (c >= 'a' && c <= 'z')
		|| (c >= '0' && c <= '9'));
}

t_env	*create_env_node(char *env_var)
{
	t_env	*new_node;
	char	*equal_sign;

	new_node = malloc(sizeof(t_env));
	if (!new_node)
		return (NULL);
	
	new_node->next = NULL;
	equal_sign = strchr(env_var, '=');
	
	if (equal_sign)
	{
		new_node->name = strndup(env_var, equal_sign - env_var);
		if (!new_node->name)
		{
			free(new_node);
			return (NULL);
		}
		new_node->value = strdup(equal_sign + 1);
		if (!new_node->value)
		{
			free(new_node->name);
			free(new_node);
			return (NULL);
		}
	}
	else
	{
		new_node->name = strdup(env_var);
		if (!new_node->name)
		{
			free(new_node);
			return (NULL);
		}
		new_node->value = NULL;
	}
	return (new_node);
}

void	free_env_list(t_env *env)
{
	t_env	*tmp;

	while (env)
	{
		tmp = env->next;
		free(env->name);
		free(env->value);
		free(env);
		env = tmp;
	}
}

void	add_to_env_list(t_env **head, t_env *new_node)
{
	t_env	*tmp;

	if (*head == NULL)
		*head = new_node;
	else
	{
		tmp = *head;
		while (tmp->next)
			tmp = tmp->next;
		tmp->next = new_node;
	}
}

t_env	*init_env(char **envp)
{
	t_env	*head;
	t_env	*new_node;
	int		i;

	head = NULL;
	i = 0;
	while (envp[i])
	{
		new_node = create_env_node(envp[i]);
		if (!new_node)
		{
			free_env_list(head); 
			return (NULL);
		}
		add_to_env_list(&head, new_node);
		i++;
	}
	return (head);
}

char *get_var(char *value, int i)
{
	int len = 0;
	int start = i;
	while (value[i] && (is_alphanumeric(value[i]) || value[i] == '_'))
	{
		len++;
		i++;
	}
	return (ft_substr(value, start, len));
}

char *get_env_var(t_env *env_list, char *name)
{
    while (env_list)
    {
        if (strcmp(env_list->name, name) == 0)
            return env_list->value;
        env_list = env_list->next;
    }
    return NULL;
}

void replace_var(t_token *tmp, int i, char *env, int len)
{
	char *start = ft_substr(tmp->value, 0, i);
	char *end = ft_strdup(tmp->value + i + 1 + len);
	char *new = ft_strjoin(start, env);
	char *value = ft_strjoin(new, end);
	free(tmp->value);
	tmp->value = value;
	free(start);
	free(end);
	free(new);
}

void to_expand(t_token *tmp, t_env *env_list)
{
	char *var_name = NULL;
	char *env_value = NULL;
	int i = 0;
	while (tmp->value[i])
	{
		if (tmp->value[i] == '$' && (tmp->value[i + 1] == '$' || tmp->value[i + 1] == '\0'))
		{
			i++;
			if (tmp->value[i] == '\0')
				break;
		}
		else if (tmp->value[i] == '$')
		{
			if (tmp->value[i + 1] == '?')
			{
				// get_exit_status();
				i++;
			}
			else
			{	
				var_name = get_var(tmp->value, i + 1);
				env_value = get_env_var(env_list, var_name);
				if (env_value)
				{
					replace_var(tmp, i, env_value, ft_strlen(var_name));
					i += ft_strlen(var_name);	
				}
				else
					replace_var(tmp, i, "", ft_strlen(var_name));
			}
			free(var_name);
		}
		i++;
	}
}

void expand_variables(t_token **token_list, t_env *env_list)
{
	if (!token_list)
		return ;
	t_token *tmp = *token_list;
	while (tmp)
	{
		if (tmp->type == 8 && tmp->next)
		{
			tmp = tmp->next;
			if (tmp->next && (tmp->type == 3 || tmp->type == 4 || tmp->next->type == 3 || tmp->next->type == 4))
				tmp->expand_heredoc = 0;
			else
				tmp->expand_heredoc = 1;
		}
		else if (tmp->type == 1 || tmp->type == 4)
			to_expand(tmp, env_list);
		tmp = tmp->next; 		
	}
}

void	print_linked_list(t_token *token_list)
{
	t_token	*current;
	t_token	*redir_tmp;
	int		i;

	current = token_list;
	while (current)
	{
		printf("token->value = %s		token->type =%d			token->has_space = %d		token->expand_heredoc = %d\n",
			current->value, current->type, current->has_space, current->expand_heredoc);
		if (current->cmds)
		{
			printf("  cmds: ");
			i = 0;
			while (current->cmds[i])
			{
				printf("[%s] ", current->cmds[i]);
				i++;
			}
			printf("\n");
		}
		if (current->redir)
		{
			printf("  redir: ");
			redir_tmp = current->redir;
			while (redir_tmp)
			{
				printf("[type:%d value:%s] ", redir_tmp->type, redir_tmp->value);
				redir_tmp = redir_tmp->next;
			}
			printf("\n");
		}
		current = current->next;
	}
}

static t_token *extract_redirections(t_token **tmp)
{
	t_token *redir_head = NULL;
	t_token *redir_tail = NULL;

	while (*tmp && ((*tmp)->type == REDIR_IN || (*tmp)->type == REDIR_OUT || (*tmp)->type == APPEND || (*tmp)->type == HEREDOC))
	{
		t_token *redir_op = *tmp;
		t_token *redir_target = redir_op->next;

		if (!redir_target)
			break;

		t_token *redir_token = create_token(redir_target->value, 0, redir_target->has_space);
		redir_token->type = redir_op->type;

		if (!redir_head)
			redir_head = redir_token;
		else
			redir_tail->next = redir_token;
		redir_tail = redir_token;

		*tmp = redir_target->next;
	}
	return redir_head;
}

static int	count_words(char const *str, char c)
{
	size_t	i;
	size_t	count;

	count = 0;
	i = 0;
	while (str[i])
	{
		if (str[i] == c)
			i++;
		else
		{
			count++;
			while (str[i] && str[i] != c)
				i++;
		}
	}
	return (count);
}

t_token *get_cmd_and_redir(t_token *token_list)
{
    t_token *final_token = NULL;
    t_token *tmp = token_list;
    t_token *pipe;

    while (tmp)
    {
        if (tmp->type != PIPE)
        {
            t_token *cmd_token = NULL;
            char **cmds = NULL;
            t_token *redir_head = NULL;
            t_token *redir_tail = NULL;
            int cmd_count = 0;
            int cmd_capacity = 8;

            cmds = malloc(sizeof(char *) * cmd_capacity);
            if (!cmds)
                return NULL;

            while (tmp && tmp->type != PIPE)
            {
                if (tmp->type == CMD || tmp->type == SINGLE_QUOTE || tmp->type == DOUBLE_QUOTE)
                {
                    if (cmd_count >= cmd_capacity)
                    {
                        cmd_capacity *= 2;
                        char **new_cmds = realloc(cmds, sizeof(char *) * cmd_capacity);
                        if (!new_cmds)
                        {
                            free(cmds);
                            return NULL;
                        }
                        cmds = new_cmds;
                    }
                    cmds[cmd_count++] = strdup(tmp->value);
                    tmp = tmp->next;
                }
                else if (tmp->type == REDIR_IN || tmp->type == REDIR_OUT || tmp->type == APPEND || tmp->type == HEREDOC)
                {
                    t_token *redir_op = tmp;
                    t_token *redir_target = tmp->next;
                    if (!redir_target)
                        break;
                    t_token *redir_token = create_token(redir_target->value, 0, redir_target->has_space);
                    redir_token->type = redir_op->type;
                    if (!redir_head)
                        redir_head = redir_token;
                    else
                        redir_tail->next = redir_token;
                    redir_tail = redir_token;
                    tmp = redir_target->next;
                }
                else
                    tmp = tmp->next;
            }
            cmds[cmd_count] = NULL;
            if (cmds && cmds[0])
            {
                cmd_token = create_token(cmds[0], 0, 0);
                cmd_token->type = CMD;
            }
            else
            {
                cmd_token = create_token("", 0, 0);
                cmd_token->type = CMD;
            }
            cmd_token->cmds = cmds;
            cmd_token->redir = redir_head;
            append_token(&final_token, cmd_token);
        }
        else if (tmp && tmp->type == PIPE)
        {
            pipe = create_token(tmp->value, 0, tmp->has_space);
            pipe->type = PIPE;
            append_token(&final_token, pipe);
            tmp = tmp->next;
        }
    }
    return final_token;
}

void	handler(int sig)
{
	(void)sig;
	write(1, "\n", 1);
	rl_on_new_line();
	rl_replace_line("", 0);
	rl_redisplay();
}

void join_tokens(t_token **tokens)
{
	t_token *tmp;
	t_token *del;
	int is_expand;
	char *new_value;
	
	tmp = *tokens;
	while (tmp)
	{
		if (tmp->has_space == 0 && tmp->next && ((tmp->type == 3 || tmp->type == 4 || tmp->type == 1) && (tmp->next->type == 3 || tmp->next->type == 4 || tmp->next->type == 1)))
		{
			is_expand = tmp->expand_heredoc;
			new_value = ft_strjoin(tmp->value, tmp->next->value);
			free(tmp->value);
			tmp->value = new_value;
			tmp->type = 1;
			tmp->expand_heredoc = is_expand;
			tmp->has_space = tmp->next->has_space;
			del = tmp->next;
			tmp->next = del->next; 
		}
		else
			tmp = tmp->next;
	}
}

void    reset_terminal_mode(void)
{
    struct termios    term;

    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    write(STDERR_FILENO, "\r\033[K", 4);
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ECHOCTL);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void	heredoc_sigint_handler(int sig)
{
	(void)sig;
	g_heredoc_interrupted = 1;
	write(1, "\n", 1);
	exit(130);
}

void	handle_heredoc_input(char *delimiter, int write_fd)
{
	char	*line;

	while (1)
	{
		line = readline("> ");
		if (!line)
		{
			write(1, " ", 1);
			break ;
		}
		if (strcmp(line, delimiter) == 0)
		{
			free(line);
			break ;
		}
		write(write_fd, line, ft_strlen(line));
		write(write_fd, "\n", 1);
		free(line);
	}
}

void process_heredoc(t_token *token)
{
	int		status;
	pid_t pid;
	int pipe_fd[2];
	t_token *redir;
	if (!token)
		return ;
	g_heredoc_interrupted = 0;
	t_token *tmp = token;
	while (tmp)
	{
		if (tmp->redir)
		{
			redir = tmp->redir;
			while (redir)
			{
				if (redir->type == HEREDOC)
				{
					if (pipe(pipe_fd) == -1)
					{
						perror("pipe failed");
						redir = redir->next;
						continue ;
					}
					pid = fork();
					if (pid == -1)
					{
						perror("fork failed");
						close(pipe_fd[0]);
						close(pipe_fd[1]);
						redir = redir->next;
						continue ;
					}
					if (pid == 0)
					{
						close(pipe_fd[0]);
						signal(SIGINT, heredoc_sigint_handler);
						signal(SIGQUIT, SIG_IGN);
						handle_heredoc_input(redir->value, pipe_fd[1]);
						close(pipe_fd[1]);
						exit(0);
					}
					close(pipe_fd[1]);
					signal(SIGINT, SIG_IGN);
					waitpid(pid, &status, 0);
					if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT)
					{
						close(pipe_fd[0]);
						redir->fd = -1;
						g_heredoc_interrupted = 1;
						signal(SIGINT, handler);
						reset_terminal_mode();
						return ;
					}
					else if (WIFEXITED(status) && WEXITSTATUS(status) == 130)
					{
						close(pipe_fd[0]);
						redir->fd = -1;
						g_heredoc_interrupted = 1;
						signal(SIGINT, handler);
						reset_terminal_mode();
						return ;
					}
					else
						redir->fd = pipe_fd[0];
					signal(SIGINT, handler);
					reset_terminal_mode();
				}
				redir = redir->next;
			}
		}
		tmp = tmp->next;	
	}
}

int	is_valid_n_flag(char *arg)
{
	int	i;

	if (!arg || arg[0] != '-' || arg[1] != 'n')
		return (0);
	i = 2;
	while (arg[i])
	{
		if (arg[i] != 'n')
			return (0);
		i++;
	}
	return (1);
}

int	ft_echo(char **cmd)
{
	int	i;
	int	newline;

	i = 1;
	newline = 1;
	while (cmd[i] && is_valid_n_flag(cmd[i]))
	{
		newline = 0;
		i++;
	}
	while (cmd[i])
	{
		printf("%s", cmd[i]);
		if (cmd[i + 1])
			printf(" ");
		i++;
	}
	if (newline)
		printf("\n");
	return (0);
}

char	*get_home_path(char **cmd, char *oldpwd)
{
	char	*path;

	if (!cmd[1] || strcmp(cmd[1], "~") == 0)
	{
		path = getenv("HOME");
		if (!path)
		{
			printf("minishell: cd: HOME not set\n");
			free(oldpwd);
			return (NULL);
		}
		return (path);
	}
	return (NULL);
}

char	*get_oldpwd_path(char **cmd, char *oldpwd)
{
	char	*path;

	if (cmd[1] && strcmp(cmd[1], "-") == 0)
	{
		path = getenv("OLDPWD");
		if (!path)
		{
			printf("minishell: cd: OLDPWD not set\n");
			free(oldpwd);
			return (NULL);
		}
		printf("%s\n", path);
		return (path);
	}
	return (NULL);
}

char	*get_envvar_path(char **cmd, char *oldpwd)
{
	char	*env_name;
	char	*path;

	if (cmd[1] && cmd[1][0] == '$')
	{
		env_name = cmd[1] + 1;
		path = getenv(env_name);
		if (!path)
		{
			printf("minishell: cd: %s: undefined variable\n", cmd[1]);
			free(oldpwd);
			return (NULL);
		}
		return (path);
	}
	return (NULL);
}

char	*get_cd_path(char **cmd, char *oldpwd)
{
	char	*path;

	path = get_home_path(cmd, oldpwd);
	if (path)
		return (path);
	path = get_oldpwd_path(cmd, oldpwd);
	if (path)
		return (path);
	path = get_envvar_path(cmd, oldpwd);
	if (path)
		return (path);
	return (cmd[1]);
}

void	update_env(char *name, char *value, t_env **env_list)
{
	t_env	*current;
	t_env	*new_env;
	char	*new_name;
	char	*new_value;

	current = *env_list;
	while (current)
	{
		if ((strcmp(current->name, name) == 0))
		{
			new_value = ft_strdup(value);
			if (!new_value)
				return ; 
			free(current->value);
			current->value = new_value;
			return ;
		}
		current = current->next;
	}
	
	new_env = malloc(sizeof(t_env));
	if (!new_env)
		return ;
	
	new_name = ft_strdup(name);
	if (!new_name)
	{
		free(new_env);
		return ;
	}
	
	new_value = ft_strdup(value);
	if (!new_value)
	{
		free(new_name);
		free(new_env);
		return ;
	}

	new_env->name = new_name;
	new_env->value = new_value;
	new_env->next = *env_list;
	*env_list = new_env;
}

void	update_pwd_vars(char *oldpwd, t_env *envlist)
{
	char	*cwd;

	cwd = getcwd(NULL, 0);
	if (!cwd)
		return ;
	update_env("OLDPWD", oldpwd, &envlist);
	update_env("PWD", cwd, &envlist);
	free(cwd);
}

int	ft_cd(char **cmd, t_env *envlist)
{
	char	*cwd;
	char	*oldpwd;
	char	*path;

	cwd = getcwd(NULL, 0);
	if (!cwd)
		return (printf("minishell: cd: error retrieving current directory\n")
			, 1);
	oldpwd = ft_strdup(cwd);
	free(cwd);
	path = get_cd_path(cmd, oldpwd);
	if (!path)
		return (1);
	if (chdir(path) != 0)
	{
		printf("minishell: cd: %s: %s\n", path, strerror(errno));
		free(oldpwd);
		return (1);
	}
	update_pwd_vars(oldpwd, envlist);
	free(oldpwd);
	return (0);
}

int	ft_pwd(void)
{
	char	*cwd;

	cwd = getcwd(NULL, 0);
	if (!cwd)
		return (perror("minishell: pwd"), 1);
	printf("%s\n", cwd);
	free(cwd);
	return (0);
}

int	is_alpha(int c)
{
	return ((c >= 'A' && c <= 'Z')
		|| (c >= 'a' && c <= 'z'));
}

int	valid_identifier(char *str)
{
	int	i;

	if (!str || !*str)
		return (0);
	if (!is_alpha(*str) && *str != '_')
		return (0);
	i = 1;
	while (str[i])
	{
		if (!is_alphanumeric(str[i]) && str[i] != '_')
			return (0);
		i++;
	}
	return (1);
}

int	parse_export_arg(char *arg, char **name, char **value, int *append)
{
	char	*equal_sign;

	*append = 0;
	equal_sign = ft_strchr(arg, '=');
	if (equal_sign == arg)
		return (printf("minishell: export: `%s': not a valid identifier\n", arg)
			, 1);
	if (equal_sign > arg + 1 && *(equal_sign - 1) == '+')
	{
		*append = 1;
		*name = ft_substr(arg, 0, equal_sign - arg - 1);
	}
	else
		*name = ft_substr(arg, 0, equal_sign - arg);
	if (!*name || !valid_identifier(*name))
	{
		if (*name)
			free(*name);
		return (printf("minishell: export: `%s': not a valid identifier\n", arg)
			, 1);
	}
	*value = ft_strdup(equal_sign + 1);
	return (0);
}

t_env	*find_env_var(char *name, t_env *env_list)
{
	t_env	*current;

	current = env_list;
	while (current)
	{
		if (strcmp(current->name, name) == 0)
			return (current);
		current = current->next;
	}
	return (NULL);
}

char	*get_env_value(char *name, t_env *env_list)
{
	t_env	*current;

	current = env_list;
	while (current)
	{
		if (strcmp(current->name, name) == 0)
			return (current->value);
		current = current->next;
	}
	return (NULL);
}

void	env_append(char *name, char *value, t_env **env_list)
{
	t_env	*found_var;
	char	*new_value;
	char	*old_value;

	found_var = find_env_var(name, *env_list);
	if (!found_var)
	{
		update_env(name, value, env_list);
		return ;
	}
	old_value = get_env_value(name, *env_list);
	if (old_value)
	{
		new_value = ft_strjoin(old_value, value);
		if (new_value)
		{
			update_env(name, new_value, env_list);
			free(new_value); 
		}
	}
	else
		update_env(name, value, env_list);
}

int	process_export(char *arg, t_env **env_list)
{
	char	*name;
	char	*value;
	int		append;

	if (parse_export_arg(arg, &name, &value, &append))
		return (1);
	if (append)
		env_append(name, value, env_list);
	else
		update_env(name, value, env_list);
	free(name);
	free(value);
	return (0);
}

int	ft_export(char **cmd, t_env **env_list)
{
	int		status;
	int		i;

	status = 0;
	i = 1;
	if (!cmd || !cmd[i])
		return (1);
	while (cmd[i])
	{
		if (!ft_strchr(cmd[i], '='))
		{
			printf("minishell: export: `%s': not a valid identifier\n", cmd[i]);
			status = 1;
		}
		else if (process_export(cmd[i], env_list) != 0)
		status = 1;
		i++;
	}
		
	return (status);
}

void	unset_var(t_env **env_list, char *name)
{
	t_env	*curr;
	t_env	*prev;

	curr = *env_list;
	prev = NULL;
	while (curr)
	{
		if (strcmp(name, curr->name) == 0)
		{
			if (!prev)
				*env_list = curr->next;
			else
				prev->next = curr->next;
			free(curr->name);
			free(curr->value);
			free(curr);
			return ;
		}
		prev = curr;
		curr = curr->next;
	}
}

int	ft_unset(char **cmd, t_env **env_list)
{
	int	i;

	i = 1;
	if (!cmd || !cmd[i])
		return (1);
	while (cmd[i])
	{
		unset_var(env_list, cmd[i]);
		i++;
	}
	return (1);
}

int	ft_env(t_env **env_list)
{
	t_env	*current;

	current = *env_list;
	while (current)
	{
		printf("%s=%s\n", current->name, current->value);
		current = current->next;
	}
	return (0);
}

int	is_digit(int c)
{
	return (c >= '0' && c <= '9');
}

int	is_num(const char *str)
{
	int	i;

	i = 0;
	if (!str || str[0] == '\0')
		return (0);
	if (str[0] == '+' || str[0] == '-')
		i++;
	if (str[i] == '\0')
		return (0);
	while (str[i])
	{
		if (!isdigit((unsigned char)str[i]))
			return (0);
		i++;
	}
	return (1);
}

int	check_exit_args(char **cmd)
{
	int	argc;

	argc = 0;
	while (cmd[argc])
		argc++;
	if (argc >= 2 && !is_num(cmd[1]))
	{
		printf("exit\nbash: exit: %s: numeric argument required\n", cmd[1]);
		exit(255);
	}
	if (argc > 2)
	{
		printf("exit\nbash: exit: too many arguments\n");
		return (-1);
	}
	if (argc == 2)
		return (atoi(cmd[1]));
	return (0);
}

int	ft_exit(char **cmd, t_env *env_list)
{
	int	status;

	status = check_exit_args(cmd);
	if (status == -1)
		return (1);
	free_env_list(env_list);
	printf("exit\n");
	exit(status);
	return (0);
}

static int	dispatch_builtin(char **cmd, t_env **envlist)
{
	if (strcmp(cmd[0], "echo") == 0)
		return (ft_echo(cmd));
	if (strcmp(cmd[0], "cd") == 0)
		return (ft_cd(cmd, *envlist));
	if (strcmp(cmd[0], "pwd") == 0)
		return (ft_pwd());
	if (strcmp(cmd[0], "export") == 0)
		return (ft_export(cmd, envlist));
	if (strcmp(cmd[0], "unset") == 0)
		return (ft_unset(cmd, envlist));
	if (strcmp(cmd[0], "env") == 0)
		return (ft_env(envlist));
	if (strcmp(cmd[0], "exit") == 0)
		return (ft_exit(cmd, *envlist));
	return (0);
}

void	write_error_no_exit(char *command, char *message)
{
	write(STDERR_FILENO, "minishell: ", 11);
	if (command)
	{
		write(STDERR_FILENO, command, strlen(command));
		write(STDERR_FILENO, ": ", 2);
	}
	write(STDERR_FILENO, message, strlen(message));
	write(STDERR_FILENO, "\n", 1);
}

int	handle_input_redir(t_token *redir)
{
	int	fd;

	fd = open(redir->value, O_RDONLY);
	if (fd == -1)
	{
		write_error_no_exit(redir->value, "No such file or directory");
		return (1);
	}
	if (dup2(fd, STDIN_FILENO) == -1)
	{
		write_error_no_exit(NULL, "dup2 failed");
		close(fd);
		return (1);
	}
	close(fd);
	return (0);
}

void	handle_heredoc_redir(t_token *redir)
{
	if (redir->fd > 0)		// >=
	{
		if (dup2(redir->fd, STDIN_FILENO) == -1)
		{
			write_error_no_exit(NULL, "dup2 failed");
			return;
		}
		close(redir->fd);
	}
}

int	handle_output_redir(t_token *redir)
{
	int	fd;

	if (redir->type == REDIR_OUT)
		fd = open(redir->value, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	else
		fd = open(redir->value, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd == -1)
	{
		write_error_no_exit(redir->value, "Permission denied");
		return (1);
	}
	if (dup2(fd, STDOUT_FILENO) == -1)
	{
		write_error_no_exit(NULL, "dup2 failed");
		close(fd);
		return (1);
	}
	close(fd);
	return (0);
}

int	handle_redirection(t_token *node)
{
	t_token	*redir;
	int		result;

	if (!node || !node->redir)
		return (0);
	redir = node->redir;
	while (redir)
	{
		if (redir->type == HEREDOC)
			handle_heredoc_redir(redir);
		else if (redir->type == REDIR_IN)
		{
			result = handle_input_redir(redir);
			if (result != 0)
				return (result);
		}
		else if (redir->type == REDIR_OUT || redir->type == APPEND)
		{
			result = handle_output_redir(redir);
			if (result != 0)
				return (result);
		}
		redir = redir->next;
	}
	return (0);
}

int	execute_builtin(t_token *node, t_env **envlist)
{
	int	stdout_backup;
	int	stdin_backup;
	int	result;
	int	redir_result;

	stdout_backup = dup(STDOUT_FILENO);
	stdin_backup = dup(STDIN_FILENO);
	
	if (node && node->redir)
	{
		redir_result = handle_redirection(node);
		if (redir_result != 0)
		{
			dup2(stdout_backup, STDOUT_FILENO);
			dup2(stdin_backup, STDIN_FILENO);
			close(stdout_backup);
			close(stdin_backup);
			return (redir_result);
		}
	}
	
	result = dispatch_builtin(node->cmds, envlist);
	
	if (node && node->redir)
	{
		dup2(stdout_backup, STDOUT_FILENO);
		dup2(stdin_backup, STDIN_FILENO);
		close(stdout_backup);
		close(stdin_backup);
	}
	
	return (result);
}

int	is_builtin(char *cmd)
{
	if (!cmd)
		return (0);
	if (strcmp(cmd, "cd") == 0)
		return (1);
	if (strcmp(cmd, "echo") == 0)
		return (1);
	if (strcmp(cmd, "exit") == 0)
		return (1);
	if (strcmp(cmd, "env") == 0)
		return (1);
	if (strcmp(cmd, "export") == 0)
		return (1);
	if (strcmp(cmd, "unset") == 0)
		return (1);
	if (strcmp(cmd, "pwd") == 0)
		return (1);
	return (0);
}

void	free_env_array(char **env_array)
{
	int	i;

	i = 0;
	if (!env_array)
		return ;
	while (env_array[i])
	{
		free(env_array[i]);
		i++;
	}
	free(env_array);
}

void	free_env_array_partial(char **env_array, int i)
{
	while (--i >= 0)
		free(env_array[i]);
	free(env_array);
}

char	*build_env_string(char *name, char *value)
{
	char	*temp;
	char	*result;

	if (!name)
		return (NULL);
	if (!value)
		return (ft_strdup(name));
	temp = ft_strjoin(name, "=");
	if (!temp)
		return (NULL);
	result = ft_strjoin(temp, value);
	free(temp);
	return (result);
}

int	count_env_nodes(t_env *env_list)
{
	int	count;

	count = 0;
	while (env_list)
	{
		count++;
		env_list = env_list->next;
	}
	return (count);
}

char	**env_list_to_array(t_env *env_list)
{
	char	**env_array;
	int		count;
	int		i;
	t_env	*curr;

	if (!env_list)
		return (NULL);
	count = count_env_nodes(env_list);
	env_array = malloc(sizeof(char *) * (count + 1));
	if (!env_array)
		return (NULL);
	curr = env_list;
	i = 0;
	while (curr)
	{
		env_array[i] = build_env_string(curr->name, curr->value);
		if (!env_array[i])
			return (free_env_array_partial(env_array, i), NULL);
		curr = curr->next;
		i++;
	}
	env_array[i] = NULL;
	return (env_array);
}

void	ft_free_arr(char **arr)
{
	int	i;

	i = 0;
	while (arr[i])
	{
		free(arr[i]);
		i++;
	}
	free(arr);
}

char	*get_path(t_env *envlist)
{
	t_env	*current;

	current = envlist;
	while (current)
	{
		if (strcmp(current->name, "PATH") == 0)
			return (current->value);
		current = current->next;
	}
	return (NULL);
}

char	**get_paths(t_env **envlist)
{
	char	*path_string;
	char	**paths;

	if (!envlist || !*envlist)
		return (NULL);
	path_string = get_path(*envlist);
	if (!path_string)
	{
		perror("path not found");
		return (NULL);
	}
	paths = ft_split(path_string, ':');
	if (!paths)
		perror("path split failed");
	return (paths);
}

char	*build_path(char *path, char *cmd)
{
	char	*tmp;
	char	*full_path;

	tmp = ft_strjoin(path, "/");
	if (!tmp)
		return (NULL);
	full_path = ft_strjoin(tmp, cmd);
	free(tmp);
	return (full_path);
}

char	*check_paths(char **paths, char *cmd)
{
	char	*full_path;
	int		i;

	if ((cmd[0] == '.' && cmd[1] == '/') || cmd[0] == '/')
	{
		if (access(cmd, F_OK) == 0)
			return (ft_strdup(cmd));
		return (NULL);
	}
	i = 0;
	while (paths[i])
	{
		full_path = build_path(paths[i], cmd);
		if (!full_path)
			return (NULL);
		if (access(full_path, F_OK) == 0)
			return (full_path);
		free(full_path);
		i++;
	}
	return (NULL);
}

char	*find_cmd_path(char *cmd, t_env **envlist)
{
	char	**paths;
	char	*result;

	if (!cmd)
		return (NULL);
	paths = get_paths(envlist);
	if (!paths)
		return (NULL);
	result = check_paths(paths, cmd);
	ft_free_arr(paths);
	return (result);
}

int	execute_cmd(char **cmds, t_env *envlist, t_token *node)
{
	pid_t	pid;
	int		status;
	char	*full_path;
	char	**env_array;
	int		redir_result;

	if (!cmds || !cmds[0] || cmds[0][0] == '\0')
		return (0);
	pid = fork();
	if (pid == -1)
	{
		write_error_no_exit(cmds[0], "fork failed");
		return (1);
	}
	if (pid == 0)
	{
		if (node && node->redir)
		{
			redir_result = handle_redirection(node);
			if (redir_result != 0)
				exit(redir_result);
		}	
		full_path = find_cmd_path(cmds[0], &envlist);
		if (!full_path)
		{
			write_error_no_exit(cmds[0], "command not found");
			exit(127);
		}
		if (access(full_path, X_OK) != 0)
		{
			write_error_no_exit(cmds[0], "Permission denied");
			free(full_path);
			exit(126);
		}
		env_array = env_list_to_array(envlist);
		if (!env_array)
		{
			free(full_path);
			write_error_no_exit(cmds[0], "environment conversion failed");
			exit(1);
		}	
		execve(full_path, cmds, env_array);
		free(full_path);
		free_env_array(env_array);
		write_error_no_exit(cmds[0], "command not found");
		exit(127);
	}
	waitpid(pid, &status, 0);
	
	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		return (128 + WTERMSIG(status));
	return (1);
}

int	execute_pipe(t_token *token, t_env **env_list, int last_status)
{
	int		fd[2];
	pid_t	pid1, pid2;
	int		status;

	if (!token || !token->next)
		return (1);

	if (pipe(fd) == -1)
		return (write_error_no_exit(NULL, "pipe failed"), 1);

	pid1 = fork();
	if (pid1 == -1)
		return (write_error_no_exit(NULL, "fork failed"), 1);

	if (pid1 == 0)
	{
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);
		if (is_builtin(token->cmds[0]))
			exit(execute_builtin(token, env_list));
		else
			exit(execute_cmd(token->cmds, *env_list, token));
	}

	pid2 = fork();
	if (pid2 == -1)
		return (write_error_no_exit(NULL, "fork failed"), 1);

	if (pid2 == 0)
	{
		close(fd[1]);
		dup2(fd[0], STDIN_FILENO);
		close(fd[0]);
		if (is_builtin(token->next->cmds[0]))
			exit(execute_builtin(token->next, env_list));
		else
			exit(execute_cmd(token->next->cmds, *env_list, token->next));
	}

	close(fd[0]);
	close(fd[1]);
	waitpid(pid1, NULL, 0);
	waitpid(pid2, &status, 0);

	if (WIFEXITED(status))
		return (WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
		return (128 + WTERMSIG(status));
	return (1);
}


int execute_cmds(t_token *token, t_env **env_list, int last_status)
{
	int status = 0;
	if (!token)
		return 1;
	t_token *tmp = token;
	while (tmp)
	{
		if (tmp->type == 1 || tmp->type == 3 || tmp->type == 4)
		{
			if (is_builtin(tmp->cmds[0]))
				status = execute_builtin(token, env_list);
			else
				status = execute_cmd(token->cmds, *env_list, token);
		}
		else if (tmp->type == 2)
			status = execute_pipe(token, env_list, last_status);
		tmp = tmp->next;
	}
	return status;
}

int main(int ac, char **av, char **env)
{
	int last_exit_status = 0;
	t_lexer *lexer = NULL;
	t_token *token = NULL;
	t_token *token_list = NULL;
	t_token *final_token = NULL;
	char *input = NULL;
	(void)ac;
	(void)av;
	t_env *env_list = init_env(env);
	while (1)
	{
		input = readline("minishell> ");
		if (!input)
		{
			write(1, "exit\n", 5);
			break;
		}
			if (input[0] == '\0')
		{
			free(input);
			continue;
		}
		add_history(input);
		token_list = NULL;
		lexer = initialize_lexer(input);
		while (lexer->position < lexer->length)
		{
			token = get_next_token(lexer);
			if (!token)
			continue;
			token->type = token_type(token);
			append_token(&token_list, token);
		}
		free_lexer(lexer);
		lexer = NULL;
		if (!token_list)
		{
			free(input);
			continue;
		}
		if (check_errors(token_list) == 1)
		{
			free(input);
			free_token_list(token_list);
			continue;
		}
		expand_variables(&token_list, env_list);
		join_tokens(&token_list);
		final_token = get_cmd_and_redir(token_list);
		process_heredoc(final_token);
		last_exit_status = execute_cmds(final_token, &env_list, last_exit_status);
		// print_linked_list(final_token);
	}
}