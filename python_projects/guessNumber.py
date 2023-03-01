from tkinter import *
from random import randint

root = Tk()

root.title('Угадай число')
root.resizable(False, False)
root.geometry('600x250')

welcome = Label(text='Игра "Угадай число"\n', font='40')
welcome.pack()

enter_num = Label(text='Загадано число от 1 до 30, введите его ниже, чтобы отгадать:\n', font='10')
enter_num.pack()


def getNumEnter():  # Функция проверки введённых данных
    num = numEnter_field.get()
    if num.isdigit() and 0 < int(num) < 30:
        return int(num)
    else:
        result.config(text='Непонятное число, введите целое число от 1 до 30!')


numEnter_field = Entry(root, font='70', justify=CENTER)
numEnter_field.pack()

result = Label(root, font='50', justify=CENTER, fg='red')
result.pack()


def game():
    global try_count

    num = getNumEnter()
    if num is None:  # Функция ничего не вернула; была ошибка ввода
        return

    if num < rand_num:
        result.config(text='Загаданное число больше, попробуйте ещё раз')
        numEnter_field.delete(0, END)
        try_count += 1
    elif num > rand_num:
        result.config(text='Загаданное число меньше, попробуйте ещё раз')
        numEnter_field.delete(0, END)
        try_count += 1
    elif num == rand_num:
        result.config(text=f'Вы угадали! Количество ваших попыток: {try_count}')


btnRead = Button(root, height=2, width=10, text="Угадать", command=game)
btnRead.pack()

try_count = 1
rand_num = randint(1, 30)

root.mainloop()