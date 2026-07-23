.PHONY: all clean rebuild run

all:
	@mkdir -p build_cmake
	@cd build_cmake && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build . --parallel $$(nproc)

clean:
	@rm -rf build_cmake
	@rm -rf build/release/y_lang_new

rebuild: clean all

run: all
	@./build/release/y_lang_new
