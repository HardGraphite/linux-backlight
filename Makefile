CC ?= gcc
CFLAGS += -Wall -Wextra -O2 -s -DNDEBUG

SRC = backlight.c
BIN ?= ${SRC:.c=}

INSTALL_DIR ?= .
INSTALLED_BIN = $(patsubst %/,%,${INSTALL_DIR})/${BIN}

${BIN}: ${SRC}
	${CC} ${CFLAGS} -o ${BIN} ${SRC}

.PHONY: install
install: ${BIN}
	[[ "${BIN}" -ef "${INSTALLED_BIN}" ]] || cp ${BIN} "${INSTALL_DIR}"
	chown root:root "${INSTALLED_BIN}"
	chmod u+s "${INSTALLED_BIN}"
	@echo ${BIN} 'has been installed to' ${INSTALL_DIR}

.PHONY: uninstall
uninstall: ${INSTALLED_BIN}
	rm "${INSTALLED_BIN}"

.PHONY: clean
clean:
	rm ${BIN}
