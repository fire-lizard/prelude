CC = g++
TARGET = bin/prelude

INCLUDEDIR = -IPrelude 
# game has too many warnings, I disabled a few here, but ideally we would fix it
CXXFLAGS = -ggdb -O0 -fmax-errors=5 -m32 -D_GNU_SOURCE -DOPENGL -DBACKPORT_18 -Wno-format -Wno-conversion-null $(INCLUDEDIR)

OBJS_SRC = $(wildcard Source/*.cpp)
OBJS_SRC += $(wildcard Prelude/*.cpp)

OBJS = $(OBJS_SRC:.cpp=.o)
STATIC_LIBS = linux/libglfw3.a


LIBDIR = -Llinux
LIBS = -lpthread -lrt -lXrandr -lXxf86vm -lXi -lXinerama -lX11 -ldl -lm -lGL -lGLEW -lbass -lssl -lcrypto

$(TARGET): $(OBJS)
		$(CC) -m32 -o $(TARGET) $(OBJS) $(STATIC_LIBS) $(LIBDIR) $(LIBS)

clean:
		rm -f $(OBJS) $(TARGET)


