# Use uma imagem base leve e oficial do Node.js
FROM node:18-alpine

# Define o diretório de trabalho dentro do container
WORKDIR /app

# Copia os arquivos de dependência e instala (isso aproveita o cache do Docker)
COPY package*.json ./
RUN npm install

# Copia o resto do código da sua aplicação
COPY . .

# O Prisma precisa gerar o cliente específico para o ambiente do container
RUN npx prisma generate

# Expõe a porta que sua aplicação usa (a mesma do server.ts)
EXPOSE 3000

# Comando para iniciar sua aplicação em modo de desenvolvimento
CMD [ "npm", "run", "dev" ]