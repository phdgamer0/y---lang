.PHONY: all clean rebuild run

all:
	@mkdir -p build_cmake
	@cd build_cmake && cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .. && cmake --build . --parallel $$(nproc)
	@cp build_cmake/compile_commands.json . 2>/dev/null || true

clean:
	@rm -rf build_cmake
	@rm -rf build/release/y_lang_new

rebuild: clean all

run: all
	@./build/release/y_lang_new
