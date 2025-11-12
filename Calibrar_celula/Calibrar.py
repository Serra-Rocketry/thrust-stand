data = []
with open('Dados-Teste-Estatico/Calibrar_celula/Dados.txt', 'r') as arq:
    lines = arq.read().split('\n')
    data = [float(line) for line in lines if line]

soma = sum(data)
media = soma / len(data)

peso = float(input('Digite o peso real do objeto: '))

calibracao = media / peso

print('O fator de calibração é: ', calibracao)
