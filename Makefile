NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -g -std=c++98

SRCDIR = src
OBJDIR = obj
INCDIR = include

SOURCES = main.cpp \
          Location.cpp \
          ServerConfig.cpp \
          ConfigParser.cpp \
          ConfigTokenizer.cpp \
          ConfigValidator.cpp \
          ClientData.cpp \
          WebServer.cpp \
          SocketManager.cpp \
          ConnectionHandler.cpp

OBJECTS = $(SOURCES:%.cpp=$(OBJDIR)/%.o)

# Header dependencies
HEADERS = $(INCDIR)/Location.hpp \
          $(INCDIR)/ServerConfig.hpp \
          $(INCDIR)/ConfigParser.hpp \
          $(INCDIR)/ConfigTokenizer.hpp \
          $(INCDIR)/ConfigValidator.hpp \
          $(INCDIR)/ClientData.hpp \
          $(INCDIR)/WebServer.hpp \
          $(INCDIR)/SocketManager.hpp \
          $(INCDIR)/ConnectionHandler.hpp

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJECTS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

# Test targets
test: $(NAME)
	@echo "Testing config parser..."
	./$(NAME) test.conf

# Create a simple test configuration
test.conf:
	@echo "Creating test configuration file..."
	@echo "server {" > test.conf
	@echo "    listen 127.0.0.1:8080;" >> test.conf
	@echo "    server_name localhost;" >> test.conf
	@echo "    error_page 404 /404.html;" >> test.conf
	@echo "    client_max_body_size 1m;" >> test.conf
	@echo "" >> test.conf
	@echo "    location / {" >> test.conf
	@echo "        root /var/www/html;" >> test.conf
	@echo "        allow_methods GET POST;" >> test.conf
	@echo "        index index.html;" >> test.conf
	@echo "        autoindex off;" >> test.conf
	@echo "    }" >> test.conf
	@echo "}" >> test.conf

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(NAME)

# Sanitizer builds
sanitize: CXXFLAGS += -fsanitize=address -g
sanitize: $(NAME)

.PHONY: all clean fclean re test debug sanitize