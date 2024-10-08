build-sharpen: ./a.out

./a.out: image_sharpener.cpp libppm.cpp
	g++ -g image_sharpener.cpp libppm.cpp

run-sharpen: ./a.out
	bash average_times.sh $(INPUT) $(OUTPUT)

./im1.out: im1.cpp libppm.cpp
	g++ -g im1.cpp libppm.cpp -o im1.out

part1: ./im1.out
	./im1.out ../images/4.ppm output_part1.ppm

clean:
	rm a.out
	rm im1.out
