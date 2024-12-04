#ifndef TRADESYNTH_SERVER_HANDLERS_H
#define TRADESYNTH_SERVER_HANDLERS_H

// Message handlers
int process_order(ServerContext* context, const Order* order);
int broadcast_market_data(ServerContext* context, const MarketData* market_data);
int process_trade_execution(ServerContext* context, const TradeExecution* trade);

#endif // TRADESYNTH_SERVER_HANDLERS_H
