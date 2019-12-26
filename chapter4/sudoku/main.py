import timeit
from threading import Thread


class Sudoku:
    """
    Sudoku Simple Class
    is handle 2 major parts
    parsing file and getting different parts of a sudoku

    it can return a square by giving it i, j
    it can return a row by giving it index
    it can return a column by giving it index
    """

    def __init__(self, file_path):
        self._columns = []
        self._rows = []

        self._parse_file(file_path)

    def _parse_file(self, file_path):
        with open(file_path, 'r') as file:
            lines = file.readlines()
            self._rows = [row.split() for row in lines]

            for i in range(9):
                column = []
                for j in range(9):
                    column.append(self._rows[j][i])

                self._columns.append(column)

    def get_column(self, index):
        """
        1 Base

        :return list with items
        """
        return self._columns[index - 1]

    def get_row(self, index):
        """
        1 Base

        :return list with items
        """
        return self._rows[index - 1]

    def get_square(self, r, c):
        """
        1 Base

         _____________________
        |1 , 1 | 1 , 2 | 1 , 3|
        |_____________________|
        |2 , 1 | 2 , 2 | 2 , 3|
        |_____________________|
        |3 , 1 | 3 , 2 | 3 , 3|
        |_____________________|

        :return list with sudoku items
        """
        r = (r - 1) * 3
        c = (c - 1) * 3

        square = []
        for i in range(r, r + 3):
            for j in range(c, c + 3):
                square.append(self._rows[i][j])

        return square


def validate_row(row):
    """
    Validate if given list is consists of 1-9 numbers
    """
    return [str(i) for i in range(1, 10)] == sorted(row)


def rows_validation_time(sudoku):
    """
    Validate All Rows and calculate it's time
    """
    global is_valid_sudoku

    t = timeit.Timer()
    for i in range(1, 10):
        is_valid_row = validate_row(sudoku.get_row(i))

        if is_valid_sudoku:
            is_valid_sudoku = is_valid_row

    print('Rows Thread:', t.timeit(1))


def columns_validation_time(sudoku):
    """
    Validate All Columns and calculate it's time
    """
    global is_valid_sudoku

    t = timeit.Timer()
    for i in range(1, 10):
        is_valid_column = validate_row(sudoku.get_column(i))
        if is_valid_sudoku:
            is_valid_sudoku = is_valid_column

    print('Columns Thread:', t.timeit(1))


def square_validation_time(sudoku):
    """
    Validate All Squares and calculate it's time
    """
    global is_valid_sudoku
    t = timeit.Timer()
    for i in range(1, 4):
        for j in range(1, 4):
            is_valid_square = validate_row(sudoku.get_square(i, j))

            if is_valid_sudoku:
                is_valid_sudoku = is_valid_square

    print('Square Thread:', t.timeit(1))


is_valid_sudoku = True

if __name__ == '__main__':
    file_path = input("File Path: ")
    sudoku = Sudoku(file_path)

    timer = timeit.Timer()

    row_validating_thread = Thread(target=rows_validation_time, args=(sudoku,))
    columns_validating_thread = Thread(target=columns_validation_time, args=(sudoku,))
    square_validating_thread = Thread(target=square_validation_time, args=(sudoku,))

    row_validating_thread.start()
    columns_validating_thread.start()
    square_validating_thread.start()

    row_validating_thread.join()
    columns_validating_thread.join()
    square_validating_thread.join()

    is_valid_sudoku = 'valid' if is_valid_sudoku else 'invalid'
    print('result: {} solution'.format(is_valid_sudoku))
