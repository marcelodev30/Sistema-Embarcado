import express, { response } from 'express';
import * as env from 'dotenv'
import { request } from 'http';

env.config();
const app = express();
const port = process.env.PORT

app.use(express.json());

app.get('/', (request, response) => {
    return response.json({ massege: 'ola porra' })
});

app.listen(port, () => {
    console.log(`Server is running on http://localhost:${port}`);

});
