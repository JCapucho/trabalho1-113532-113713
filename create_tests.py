import subprocess


def create_big_pictures():
    sizes = [20, 40, 80]
    for width in sizes:
        for height in sizes:
            subprocess.run(["./imageTool", "create", f"{width},{height}",
                            "save", f"test/locate/big_{width}x{height}.pgm"])


def create_small_pictures():
    subprocess.run(["./imageTool", "create", "1,1", "neg", "save", "test/locate/white_pixel.pgm"])
    sizes = [2, 4, 8, 16]
    for width in sizes:
        for height in sizes:
            name = f"test/locate/small_{width}x{height}.pgm"
            subprocess.run(["./imageTool", "create", f"{width},{height}",
                            "save", name])
            subprocess.run(["./imageTool", "test/locate/white_pixel.pgm", name,
                           "paste", f"{width - 1},{height - 1}", "save", name])


def create_small_picture_array():
    sizes = [2, 4, 8, 16]
    for width in sizes:
        for height in sizes:
            small_images.append(f"test/locate/small_{width}x{height}.pgm")


def create_big_picture_array():
    sizes = [20, 40, 80]
    for width in sizes:
        for height in sizes:
            big_images.append(f"test/locate/big_{width}x{height}.pgm")


def runtest(arg_array):
    subprocess.run(arg_array, stdout=output)


def make_tests():
    for big_image in big_images:
        for small_image in small_images:
            runtest(["./imageTool", small_image, big_image, "locate"])


def parse_results():
    file = open("test/locate/1.out", "r")
    lines = []
    for line in file:
        lines.append(line)
    result = open("test/locate/1.out", "w")
    for idx, line in enumerate(lines):
        if idx % 2 == 0:
            print(line, file=result, end="")


if __name__ == "__main__":
    output = open("test/locate/1.out", "wb")
    big_images = []
    small_images = []
    create_big_picture_array()
    create_small_picture_array()
    make_tests()
    parse_results()
