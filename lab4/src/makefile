# Targets to compile and run the sharpen application
build-sharpen: ./a.out

./a.out: image_sharpener.cpp libppm.cpp
	g++ -g image_sharpener.cpp libppm.cpp -o a.out

run-sharpen: ./a.out
	bash average_times.sh $(INPUT) $(OUTPUT)

# Target to compile and run part1
./im1.out: im1.cpp libppm.cpp
	g++ -g im1.cpp libppm.cpp -o im1.out

part1: ./im1.out
	./im1.out ../images/1.ppm output_part1.ppm

# Target to compile and run part2_1
./im2.out: im2.cpp libppm.cpp
	g++ -g im2.cpp libppm.cpp -o im2.out

part2_1: ./im2.out
	./im2.out ../images/1.ppm output_part2_1.ppm
# Target to compile and run part2_2
./im2_2.out: im2_2.cpp libppm.cpp
	g++ -g im2_2.cpp libppm.cpp -o im2_2.out

part2_2: ./im2_2.out
	./im2_2.out ../images/1.ppm output_part2_2.ppm
clean:
	rm -f a.out
	rm -f im1.out
	rm -f im2.out
	rm -f im2_2.out
