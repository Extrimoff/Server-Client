#include "Server.hpp"
#include "../../Utils/Json.hpp"
#include "../../Utils/base64.hpp"
#include "../../Network/PacketManager/PacketManager.hpp"

#include <openssl/x509.h>
#include <openssl/pem.h>
#include <bcrypt_.h>
#include <mstcpip.h>
#include <print>

static bool createSelfSignedCert(SSL_CTX* ctx) {
    bool result = false;

    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* pctx = nullptr;
    X509* x509 = nullptr;

    do {
        // Создаем контекст для генерации ключа RSA
        pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
        if (!pctx) break;

        if (EVP_PKEY_keygen_init(pctx) <= 0) break;
        if (EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 2048) <= 0) break;

        // Генерируем ключ
        if (EVP_PKEY_keygen(pctx, &pkey) <= 0) break;

        // Создаем сертификат
        x509 = X509_new();
        if (!x509) break;

        // Серийный номер
        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);

        // Устанавливаем дату начала действия
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        // Устанавливаем дату окончания (100 лет вперед)
        X509_gmtime_adj(X509_get_notAfter(x509), 31536000L * 100);

        // Привязываем публичный ключ
        X509_set_pubkey(x509, pkey);

        // Заполняем имя субъекта сертификата
        X509_NAME* name = X509_get_subject_name(x509);
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char*)"RU", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (const unsigned char*)"MyCompany", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)"localhost", -1, -1, 0);

        // issuer == subject (самоподписанный)
        X509_set_issuer_name(x509, name);

        // Подписываем сертификат
        if (!X509_sign(x509, pkey, EVP_sha256())) break;

        // Загружаем сертификат и ключ в SSL_CTX
        if (SSL_CTX_use_certificate(ctx, x509) != 1) break;
        if (SSL_CTX_use_PrivateKey(ctx, pkey) != 1) break;

        result = true;
    } while (false);

    if (pctx) EVP_PKEY_CTX_free(pctx);
    if (x509) X509_free(x509);
    if (pkey) EVP_PKEY_free(pkey);

    return result;
}

Server::Server(
    const uint16_t port,
    KeepAliveConfig ka_conf,
    unsigned int thread_count
) : m_port(port),
    m_thread_pool(thread_count),
    m_ka_conf(ka_conf),
    m_status(ServerStatus::close),
    m_db(SQLite::Database("database.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE | SQLite::OPEN_FULLMUTEX)),
    m_ssl_ctx(nullptr)
{
	if (auto err = WSAStartup(MAKEWORD(2, 2), &m_wData); err != 0) {
		char buffer[256];
		strerror_s(buffer, sizeof(buffer), err);
        std::println(stderr,
            "WSAStartup error!\n"
            "Code: {} Err: {}",
            err, buffer);
		exit(1);
	}

    this->initDatabase();

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    m_ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!m_ssl_ctx) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    SSL_CTX_set_min_proto_version(m_ssl_ctx, TLS1_2_VERSION);
    SSL_CTX_set_cipher_list(m_ssl_ctx, "HIGH:!aNULL:!MD5");

    if (!createSelfSignedCert(m_ssl_ctx)) {
        ERR_print_errors_fp(stderr);
        exit(1);
    }
}

void Server::initDatabase()
{
    try {
        // (Users)
        m_db.exec(
            "CREATE TABLE IF NOT EXISTS Users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "email TEXT UNIQUE NOT NULL, "
            "password_hash TEXT NOT NULL, "
            "first_name TEXT NOT NULL, "
            "last_name TEXT NOT NULL, "
            "phone_number TEXT, "
            "role TEXT CHECK(role IN ('guest', 'admin')) NOT NULL DEFAULT 'guest', "
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
            "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP);"
        );

        // (Rooms)
        m_db.exec(
            "CREATE TABLE IF NOT EXISTS Rooms ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "room_type TEXT NOT NULL, "
            "price_per_night REAL NOT NULL, "
            "capacity INTEGER NOT NULL, "
            "availability BOOLEAN NOT NULL DEFAULT 1, "
            "description TEXT);"
        );

        // (Bookings)
        m_db.exec(
            "CREATE TABLE IF NOT EXISTS Bookings ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "user_id INTEGER NOT NULL, "
            "room_id INTEGER NOT NULL, "
            "check_in_date DATETIME NOT NULL, "
            "check_out_date DATETIME NOT NULL, "
            "booking_date DATETIME DEFAULT CURRENT_TIMESTAMP, "
            "status TEXT CHECK(status IN ('подтверждено', 'отменено', 'завершено')) NOT NULL DEFAULT 'подтверждено', "
            "FOREIGN KEY(user_id) REFERENCES Users(id) ON DELETE CASCADE, "
            "FOREIGN KEY(room_id) REFERENCES Rooms(id) ON DELETE CASCADE);"
        );

        m_db.exec(
            "CREATE TRIGGER IF NOT EXISTS update_users_updated_at "
            "AFTER UPDATE ON Users "
            "FOR EACH ROW "
            "BEGIN "
            "UPDATE Users SET updated_at = CURRENT_TIMESTAMP WHERE id = OLD.id; "
            "END;"
        );

        m_db.exec("PRAGMA foreign_keys = ON;");        

        m_db.exec(std::format(R"(INSERT INTO Users(email, password_hash, first_name, last_name, phone_number, role) VALUES
            ('ivanov1@example.ru', '{0}', 'Иван', 'Иванов', '+79261234567', 'guest'),
            ('petrova2@example.ru', '{1}', 'Анна', 'Петрова', '+79031234568', 'admin'),
            ('sidorov3@example.ru', '{2}', 'Сидор', 'Сидоров', '+79876543210', 'guest'),
            ('maria4@example.ru', '{3}', 'Мария', 'Кузнецова', '+79101234567', 'guest'),
            ('alexey5@example.ru', '{4}', 'Алексей', 'Смирнов', '+79684561234', 'guest'),
            ('elena6@example.ru', '{5}', 'Елена', 'Орлова', '+79776543210', 'admin'),
            ('nikita7@example.ru', '{6}', 'Никита', 'Фёдоров', '+79504561233', 'guest'),
            ('darya8@example.ru', '{7}', 'Дарья', 'Морозова', '+79991231234', 'guest'),
            ('egor9@example.ru', '{8}', 'Егор', 'Алексеев', '+79098765432', 'guest'),
            ('ksenia10@example.ru', '{9}', 'Ксения', 'Громова', '+79314567890', 'guest'),
            ('andrey11@example.ru', '{10}', 'Андрей', 'Тихонов', '+79216789988', 'guest'),
            ('tatiana12@example.ru', '{11}', 'Татьяна', 'Соловьёва', '+79023456789', 'guest'),
            ('sergey13@example.ru', '{12}', 'Сергей', 'Ковалёв', '+79451237890', 'guest'),
            ('natalia14@example.ru', '{13}', 'Наталья', 'Беляева', '+79014567321', 'admin'),
            ('artem15@example.ru', '{14}', 'Артём', 'Зайцев', '+79345678901', 'guest'),
            ('olga16@example.ru', '{15}', 'Ольга', 'Калинина', '+79671234567', 'guest'),
            ('viktor17@example.ru', '{16}', 'Виктор', 'Максимов', '+79512349876', 'guest'),
            ('irina18@example.ru', '{17}', 'Ирина', 'Чернова', '+79872123456', 'guest'),
            ('vadim19@example.ru', '{18}', 'Вадим', 'Киселёв', '+79098761234', 'admin'),
            ('alisa20@example.ru', '{19}', 'Алиса', 'Мельникова', '+79112345678', 'guest'),
            ('admin@admin.ru', '{20}', 'Админ', 'Админ', '', 'admin');
            )",
                bcrypt::generateHash("qwerty123"),
                bcrypt::generateHash("password456"),
                bcrypt::generateHash("letmein789"),
                bcrypt::generateHash("12345678"),
                bcrypt::generateHash("zxcvbnm"),
                bcrypt::generateHash("passpass"),
                bcrypt::generateHash("hello123"),
                bcrypt::generateHash("mysecurepass"),
                bcrypt::generateHash("sunshine"),
                bcrypt::generateHash("trustme1"),
                bcrypt::generateHash("111222333"),
                bcrypt::generateHash("securepass"),
                bcrypt::generateHash("qazwsxedc"),
                bcrypt::generateHash("abcABC123"),
                bcrypt::generateHash("pass1234"),
                bcrypt::generateHash("1111aaaa"),
                bcrypt::generateHash("pa$$w0rd"),
                bcrypt::generateHash("mypass2023"),
                bcrypt::generateHash("adminpass"),
                bcrypt::generateHash("123qweasd"),
                bcrypt::generateHash("admin123")
            )
        );

        m_db.exec(R"(
            INSERT INTO Rooms(room_type, price_per_night, capacity, availability, description) VALUES
            ('Одноместный', 2500.00, 1, 1, 'Уютный номер для одного человека с видом на город'),
            ('Двухместный', 3900.50, 2, 1, 'Комфортабельный номер с двуспальной кроватью'),
            ('Люкс', 7800.99, 3, 1, 'Просторный номер с джакузи и балконом'),
            ('Семейный', 6200.00, 4, 1, 'Идеален для проживания с детьми'),
            ('Апартаменты', 10500.00, 5, 1, 'Полностью оборудованные апартаменты с кухней'),
            ('Студия', 4700.75, 2, 1, 'Номер открытой планировки с мини-кухней'),
            ('Одноместный', 2300.00, 1, 1, 'Экономичный вариант для короткой поездки'),
            ('Двухместный', 4100.00, 2, 0, 'Номер с двумя односпальными кроватями'),
            ('Люкс', 8200.00, 3, 1, 'Роскошный номер с отдельной гостиной зоной'),
            ('Семейный', 5900.00, 4, 1, 'Номер с двумя спальнями и гостиной'),
            ('Апартаменты', 11200.00, 5, 0, 'Большие апартаменты с современной мебелью'),
            ('Студия', 4800.00, 2, 1, 'Современный номер с дизайнерским интерьером'),
            ('Одноместный', 2100.00, 1, 1, 'Компактный номер с рабочим местом'),
            ('Двухместный', 3850.50, 2, 1, 'Стильный номер для пары'),
            ('Люкс', 7990.00, 3, 0, 'Номер с панорамными окнами и мини-баром'),
            ('Семейный', 6100.00, 4, 1, 'Номер с удобствами для детей'),
            ('Апартаменты', 10800.00, 5, 1, 'Просторные апартаменты с двумя ванными комнатами'),
            ('Студия', 4500.00, 2, 1, 'Светлая студия с современным декором'),
            ('Двухместный', 4000.00, 2, 1, 'Классический номер с телевизором и Wi-Fi'),
            ('Одноместный', 2600.00, 1, 0, 'Номер для деловой поездки с хорошим освещением');
        )");

        m_db.exec(R"(
            INSERT INTO Bookings(user_id, room_id, check_in_date, check_out_date, status) VALUES
            (1, 3, '2025-06-10', '2025-06-15', 'подтверждено'),
            (2, 5, '2025-07-01', '2025-07-07', 'подтверждено'),
            (3, 1, '2025-05-25', '2025-05-30', 'завершено'),
            (4, 2, '2025-06-05', '2025-06-08', 'отменено'),
            (5, 4, '2025-06-20', '2025-06-25', 'подтверждено'),
            (6, 7, '2025-06-15', '2025-06-17', 'завершено'),
            (7, 6, '2025-07-10', '2025-07-15', 'подтверждено'),
            (8, 8, '2025-08-01', '2025-08-05', 'подтверждено'),
            (9, 2, '2025-06-12', '2025-06-13', 'отменено'),
            (10, 1, '2025-07-20', '2025-07-25', 'подтверждено'),
            (11, 3, '2025-05-28', '2025-05-30', 'завершено'),
            (12, 4, '2025-06-18', '2025-06-22', 'подтверждено'),
            (13, 5, '2025-07-05', '2025-07-10', 'подтверждено'),
            (14, 6, '2025-08-15', '2025-08-20', 'подтверждено'),
            (15, 7, '2025-06-25', '2025-06-30', 'завершено'),
            (16, 8, '2025-07-02', '2025-07-06', 'отменено'),
            (17, 2, '2025-07-12', '2025-07-14', 'подтверждено'),
            (18, 3, '2025-06-09', '2025-06-11', 'подтверждено'),
            (19, 1, '2025-07-01', '2025-07-03', 'завершено'),
            (20, 5, '2025-08-10', '2025-08-12', 'подтверждено');
        )");

    }
    catch (const SQLite::Exception& e) {
        std::println(stderr, "Exception: {}", e.what());
        exit(1);
    }
}

Server::~Server() {
	if (m_status == ServerStatus::up)
		stop();
	WSACleanup();
}

bool Server::enableKeepAlive(SOCKET socket) {
    int flag = 1;

    tcp_keepalive ka{ 1, m_ka_conf.ka_idle * 1000, m_ka_conf.ka_intvl * 1000 };
    if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&flag), sizeof(flag)) != 0) return false;
    unsigned long numBytesReturned = 0;
    if (WSAIoctl(socket, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &numBytesReturned, 0, nullptr) != 0) return false;

    return true;
}

void Server::handlingAcceptLoop() {
    int addrlen = sizeof(SOCKADDR_IN);
    SOCKADDR_IN client_addr;
    if (SOCKET client_socket = accept(m_serv_socket, reinterpret_cast<struct sockaddr*>(&client_addr), &addrlen);
        client_socket != INVALID_SOCKET && m_status == ServerStatus::up) {  

        if (enableKeepAlive(client_socket)) {
            SSL* ssl = SSL_new(m_ssl_ctx);
            SSL_set_fd(ssl, client_socket);

            if (SSL_accept(ssl) <= 0) {
                ERR_print_errors_fp(stderr);
                SSL_free(ssl);
                shutdown(client_socket, SD_BOTH);
                closesocket(client_socket);
                return;
            }

            std::unique_ptr<RemoteClient> client = std::make_unique<RemoteClient>(client_socket, client_addr, ssl);
            client->onConnect();
            std::lock_guard<std::mutex> lock(m_client_mutex); 
            m_client_list.emplace(std::move(client));
        }
        else {
            shutdown(client_socket, SD_BOTH);
            closesocket(client_socket);
        }
    }

    if (m_status == ServerStatus::up)
        m_thread_pool.addJob([this]() { handlingAcceptLoop(); });
}

void Server::dataReceivingLoop() {
    [this] {
        std::lock_guard lock(m_client_mutex);
        for (auto it = m_client_list.begin(); it != m_client_list.end();) {

            auto& client = *it;

            if (const auto& data = client->receiveData(); !data.empty()) {
                m_thread_pool.addJob([this, _data = std::move(data), &client = *client] {

                    std::lock_guard client_lock(client.m_access_mtx);

                    auto badPacket_func = [&client] {
                        Packet pckt;
                        client.sendData(pckt);
                        client.disconnect();
                        std::println(stderr, "Bad Packet Error");
                    };

                    try {
                        std::string rawData(_data.begin(), _data.end());
                        rawData = base64::from_base64(rawData);

                        nlohmann::json data = nlohmann::json::parse(rawData);

                        auto packet = PacketManager::CreatePacket(data);
                        if (packet->getID() == PacketID::Unknown) { badPacket_func(); }

                        std::println("Handling packet: {} from {}", packet->toString(), client.clientData.login);
                        packet->handlePacket(*this, client);
                    }
                    catch (...) {
                        badPacket_func();
                    }
                    });
            }

            if (client->m_status == SocketStatus::disconnected &&
                !client->isDisconnecting.exchange(true)) {

                //auto client_node = m_client_list.extract(it++);
                m_thread_pool.addJob([this, it = it++]() {
                    std::lock_guard list_lock(m_client_mutex);

                    auto client_node = m_client_list.extract(it);
                    auto&& client = std::move(client_node.value());
                    
                    std::lock_guard client_lock(client->m_access_mtx);
                    client->onDisconnect();
                });

                continue;  
            }
            else {
                ++it; 
            }
        }
        }();

    if (m_status == ServerStatus::up)
        m_thread_pool.addJob(std::bind(&Server::dataReceivingLoop, this));
}

ServerStatus Server::start() {
    int flag;
    if (m_status == ServerStatus::up) stop();

    SOCKADDR_IN address;
    address.sin_addr.S_un.S_addr = INADDR_ANY;
    address.sin_port = htons(m_port);
    address.sin_family = AF_INET;


    if ((m_serv_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        return m_status = ServerStatus::err_socket_init;

    if (unsigned long mode = 0; ioctlsocket(m_serv_socket, FIONBIO, &mode) == SOCKET_ERROR) {
        return m_status = ServerStatus::err_socket_init;
    }

    if (flag = true;
        (setsockopt(m_serv_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&flag), sizeof(flag)) == -1) ||
        (bind(m_serv_socket, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR))
        return m_status = ServerStatus::err_socket_bind;

    if (listen(m_serv_socket, SOMAXCONN) == SOCKET_ERROR)
        return m_status = ServerStatus::err_socket_listening;

    m_status = ServerStatus::up;
    m_thread_pool.addJob(std::bind(&Server::handlingAcceptLoop, this));
    m_thread_pool.addJob(std::bind(&Server::dataReceivingLoop, this));
    return m_status;
}

void Server::stop() {
    m_thread_pool.dropUnstartedJobs();
    m_status = ServerStatus::close;
    closesocket(m_serv_socket);
    m_client_list.clear();
}

bool Server::connectTo(uint32_t host, uint16_t port) {
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (client_socket == INVALID_SOCKET) return false;

    SOCKADDR_IN address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = host;
    address.sin_port = htons(port);

    if (connect(client_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        closesocket(client_socket);
        return false;
    }

    if (!enableKeepAlive(client_socket)) {
        shutdown(client_socket, SD_BOTH);
        closesocket(client_socket);
        return false;
    }

    SSL* ssl = SSL_new(m_ssl_ctx);
    if (!ssl) {
        closesocket(client_socket);
        return false;
    }

    SSL_set_fd(ssl, static_cast<int>(client_socket));

    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        closesocket(client_socket);
        return false;
    }

    auto client = std::make_unique<RemoteClient>(client_socket, address, ssl);
    client->onConnect();

    std::scoped_lock lock(m_client_mutex);
    m_client_list.emplace(std::move(client));

    return true;
}

void Server::sendData(Packet const& packet) {
    m_client_mutex.lock();
    for (const auto& client : m_client_list) client->sendData(packet);
    m_client_mutex.unlock();
}

bool Server::sendDataBy(uint32_t host, uint16_t port, Packet const& packet) {
    m_client_mutex.lock();
    bool is_data_sent = false;
    if (auto client_it = m_client_list.find(ClientKey{ host, port }); client_it != m_client_list.cend()) {
        (*client_it)->sendData(packet);
        is_data_sent = true;
    } 
    m_client_mutex.unlock();
    return is_data_sent;
}

bool Server::disconnectBy(uint32_t host, uint16_t port) {
    m_client_mutex.lock();
    bool client_is_disconnected = false;
    for (const auto& client : m_client_list)
        if (client->getHost() == host && client->getPort() == port) {
            client->disconnect();
            client_is_disconnected = true;
            break;
        }
    m_client_mutex.unlock();
    return client_is_disconnected;
}

void Server::disconnectAll() {
    m_client_mutex.lock();
    for (const auto& client : m_client_list)
        client->disconnect();
    m_client_mutex.unlock();
}